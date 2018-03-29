/* checksum.c -- checksum calculation and checksum file management
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* prototypes */
#include "checksum.h"
/* error handling */
#include "error.h"
#include <errno.h>
#include <openssl/err.h>
/* read_file() */
#include "readfile.h"
/* sorting the file */
#include "checksumsort.h"
/* FILE* */
#include <stdio.h>
/* digest algorithms */
#include <openssl/evp.h>
/* strstr() */
#include <string.h>
/* file size + does_file_exist() */
#include <sys/stat.h>
/* does_file_exist() */
#include <errno.h>

/* debugging purposes */
#include <assert.h>

/* 16mb */
#define MAX_RUN_SIZE (1 << 24)

int bytes_to_hex(const unsigned char* bytes, unsigned len, char** out){
	unsigned i;
	unsigned outptr;
	unsigned outlen;
	const char hexmap[] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F'};

	if (!out || !bytes){
		log_error(__FL__, STR_ENULL);
	}

	/* 2 hex chars = 1 byte */
	/* +1 for null terminator */
	outlen = len * 2 + 1;
	*out = malloc(outlen);
	if (!out){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}

	for (i = 0, outptr = 0; i < len; ++i, outptr += 2){
		/* NOTE TO SELF:
		 * (*out)[x] != *out[x] */

		/* converts top 4 bits to a hex digit */
		(*out)[outptr] = hexmap[(bytes[i] & 0xF0) >> 4];
		/* converts bottom 4 bits */
		(*out)[outptr + 1] = hexmap[(bytes[i] & 0x0F)];
	}
	/* since '\0' is not a valid hex char, we can null-terminate */
	(*out)[outlen - 1] = '\0';

	return 0;
}

/*
   static int hex_to_bytes(char* hex, unsigned len, unsigned char** out){
   unsigned ptr;
   unsigned hexptr;
   unsigned c;
// 2 hex digits = 1 byte //
out = malloc(len / 2);
if (!out){
return ERR_OUT_OF_MEMORY;
}

for (ptr = 0, hexptr = 0; hexptr < len; ptr++, hexptr += 2){
sscanf(&(hex[hexptr]), "%2x", &c);
(*out)[ptr] = c;
}

return 0;
}
*/

/* computes a hash
 * returns 0 on success or err on failure */
int checksum(const char* file, const char* algorithm, unsigned char** out, unsigned* len){
	FILE* fp = NULL;
	EVP_MD_CTX* ctx = NULL;
	const EVP_MD* md;
	unsigned char buffer[BUFFER_LEN];
	int length;
	int ret = 0;

	fp = fopen(file, "rb");
	if (!fp){
		log_error(__FL__, STR_EFOPEN, file, strerror(errno));
		return -1;
	}

	if (!(ctx = EVP_MD_CTX_create())){
		log_error(__FL__, "Failed to initialize EVP_MD_CTX");
		ERR_print_errors_fp(stderr);
		ret = -1;
		goto cleanup;
	}

	/* get EVP_MD* from char* */
	if (algorithm){
		OpenSSL_add_all_algorithms();
		if (!(md = EVP_get_digestbyname(algorithm))){
			log_error(__FL__, "Failed to load digest algorithm from name");
			ERR_print_errors_fp(stderr);
			ret = -1;
			goto cleanup;
		}
	}
	/* default to sha1 if NULL algorithm */
	else{
		md = EVP_sha1();
	}

	if (EVP_DigestInit_ex(ctx, md, NULL) != 1){
		log_error(__FL__, "Failed to initialize digest algorithm");
		ERR_print_errors_fp(stderr);
		ret = -1;
		goto cleanup;
	}

	/* both of these must point to valid locations */
	if (!len || !out){
		log_error(__FL__, STR_ENULL);
		ret = -1;
		goto cleanup;
	}
	*len = EVP_MD_size(md);
	if (!(*out = malloc(EVP_MD_size(md)))){
		log_fatal(__FL__, STR_ENOMEM);
		ret = -1;
		goto cleanup;
	}

	while ((length = read_file(fp, buffer, sizeof(buffer))) > 0){
		if (ferror(fp)){
			log_error(__FL__, STR_EFREAD, fp);
			goto cleanup;
		}
		if (EVP_DigestUpdate(ctx, buffer, length) != 1){
			log_error(__FL__, "Failed to calculate checksum");
			ERR_print_errors_fp(stderr);
			goto cleanup;
		}
	}

	if (EVP_DigestFinal_ex(ctx, *out, len) != 1){
		log_error(__FL__, "Failed to finalize checksum calculation");
		goto cleanup;
	}

cleanup:
	if (ctx){
		EVP_MD_CTX_destroy(ctx);
	}
	if (fp){
		fclose(fp);
	}
	return ret;
}

static int file_to_element(const char* file, const char* algorithm, element** out){
	unsigned char* buffer = NULL;
	unsigned len = 0;
	int ret = 0;

	if (!file || !out){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	*out = malloc(sizeof(**out));
	if (!out){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}
	(*out)->file = malloc(strlen(file) + 1);
	(*out)->checksum = NULL;
	if (!(*out)->file){
		log_fatal(__FL__, STR_ENOMEM);
		ret = -1;
		goto cleanup;
	}
	strcpy((*out)->file, file);

	/* compute checksum */
	if (checksum(file, algorithm, &buffer, &len) != 0){
		log_debug(__FL__, "checksum() in file_to_element() did not return 0");
		ret = -1;
		goto cleanup;
	}

	/* convert it to hex */
	if (bytes_to_hex(buffer, len, &(*out)->checksum) != 0){
		log_warning(__FL__, "Failed to convert raw checksum to hexadecimal");
		ret = -1;
		goto cleanup;
	}

	free(buffer);
	if (ret == 0){
		return 0;
	}

cleanup:
	if (*out){
		free((*out)->file);
		free((*out)->checksum);
		free(*out);
		*out = NULL;
	}
	free(buffer);
	return ret;
}

/* adds a checksum to a file
 * format: "[file]\0[hex hash]\0"
 *
 * the above format is because unix decided to have literally
 * every character except '/' (needed for folders) and
 * '\0' be valid in a filename. this means that the only valid
 * way to seperate the hash from its filename is to use a '\0'
 *
 * returns 0 on success or err on error */
int add_checksum_to_file(const char* file, const char* algorithm, FILE* out, FILE* prev_checksums){
	element* e;
	char* checksum = NULL;
	int ret;

	if (!file ||  !out){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	if (file_to_element(file, algorithm, &e) != 0){
		log_debug(__FL__, "Could not create element from file");
		return -1;
	}

	if (prev_checksums &&
			search_for_checksum(prev_checksums, e->file, &checksum) == 0 &&
			strcmp(checksum, e->checksum) == 0){
		ret = 1;
	}
	else{
		ret = 0;
	}
	free(checksum);

	if (write_element_to_file(out, e) != 0){
		free_element(e);
		log_debug(__FL__, "Could not write element to file");
		return -1;
	}

	free_element(e);
	return ret;
}

int sort_checksum_file(FILE* fp_in, FILE* fp_out){
	FILE** tmp_files;
	size_t n_files;

	if (create_initial_runs(fp_in, &tmp_files, &n_files) != 0){
		log_debug(__FL__, "Error creating initial runs");
		return -1;
	}
	if (merge_files(tmp_files, n_files, fp_out) != 0){
		log_debug(__FL__, "Error merging files");
		return -1;
	}

	free_filearray(tmp_files, n_files);
	return 0;
}

int search_for_checksum(FILE* fp_checksums, const char* key, char** checksum){
	return search_file(fp_checksums, key, checksum);
}

static int check_file_exists(const char* file){
	struct stat st;

	if (lstat(file, &st) == 0){
		return 1;
	}
	switch (errno){
	case ENOENT:
	case ENOTDIR:
		return 0;
	default:
		log_error(__FL__, "Could not check for existence of %s (%s)", file, strerror(errno));
		return -1;
	}
}

int create_removed_list(FILE* checksum_file, FILE* out_file){
	element* tmp;
	int err;

	rewind(out_file);

	while ((tmp = get_next_checksum_element(checksum_file)) != NULL){
		err = check_file_exists(tmp->file);
		switch (err){
		case 1:
			break;
		case 0:
			fprintf(out_file, "%s%c\n", tmp->file, '\0');
			break;
		default:
			free_element(tmp);
			return err;
		}
		free_element(tmp);
	}

	return 0;
}

char* get_next_removed(FILE* fp){
	long pos_origin;
	long pos_file;
	int c;
	size_t len_file;
	char* ret = NULL;

	if (!fp){
		log_error(__FL__, STR_ENULL);
		return NULL;
	}

	pos_origin = ftell(fp);

	while ((c = fgetc(fp)) != '\0'){
		if (c == EOF){
			log_debug(__FL__, "get_next_removed(): reached EOF");
			return NULL;
		}
	}
	pos_file = ftell(fp);
	len_file = pos_file - pos_origin;

	ret = malloc(len_file);
	if (!ret){
		log_fatal(__FL__, STR_ENOMEM);
		return NULL;
	}

	fseek(fp, pos_origin, SEEK_SET);
	fread(ret, 1, len_file, fp);
	/* advance file pointer beyond '\n' */
	fgetc(fp);

	return ret;
}
