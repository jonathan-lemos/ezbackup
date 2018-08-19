/** @file checksum.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "checksum.h"
#include "crypt/base16.h"
#include "log.h"
#include <errno.h>
#include <openssl/err.h>
#include "file/filehelper.h"
#include "checksumsort.h"
#include "strings/stringhelper.h"
#include <stdio.h>
#include <openssl/evp.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

const EVP_MD* get_evp_md(const char* hash_name){
	return hash_name ? EVP_get_digestbyname(hash_name) : EVP_md_null();
}

int bytes_to_hex(const unsigned char* bytes, unsigned len, char** out){
	return to_base16(bytes, len, out);
	/*
	unsigned i;
	unsigned outptr;
	unsigned outlen;
	const char hexmap[] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F'};

	return_ifnull(bytes, -1);
	return_ifnull(out, -1);

	/ 2 hex chars = 1 byte /
	/ +1 for null terminator /
	outlen = len * 2 + 1;
	*out = malloc(outlen);
	if (!out){
		log_enomem();
		return -1;
	}

	for (i = 0, outptr = 0; i < len; ++i, outptr += 2){
		/ NOTE TO SELF:
		  (*out)[x] != *out[x] /

		/ converts top 4 bits to a hex digit /
		(*out)[outptr] = hexmap[(bytes[i] & 0xF0) >> 4];
		/ converts bottom 4 bits /
		(*out)[outptr + 1] = hexmap[(bytes[i] & 0x0F)];
	}
	/ since '\0' is not a valid hex char, we can null-terminate /
	(*out)[outlen - 1] = '\0';

	return 0;
	*/
}

/*
   static int hex_to_bytes(char* hex, unsigned len, unsigned char** out){
   unsigned ptr;
   unsigned hexptr;
   unsigned c;
   / 2 hex digits = 1 byte /
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
int checksum(const char* file, const EVP_MD* algorithm, unsigned char** out, unsigned* len){
	FILE* fp = NULL;
	EVP_MD_CTX* ctx = NULL;
	unsigned char buffer[BUFFER_LEN];
	int length;
	int ret = 0;

	return_ifnull(file, -1);
	return_ifnull(out, -1);
	return_ifnull(len, -1);

	fp = fopen(file, "rb");
	if (!fp){
		log_efopen(file);
		return -1;
	}

	if (!(ctx = EVP_MD_CTX_create())){
		log_error("Failed to initialize EVP_MD_CTX");
		ERR_print_errors_fp(stderr);
		ret = -1;
		goto cleanup;
	}

	if (!algorithm){
		algorithm = EVP_sha1();
	}

	if (EVP_DigestInit_ex(ctx, algorithm, NULL) != 1){
		log_error("Failed to initialize digest algorithm");
		ERR_print_errors_fp(stderr);
		ret = -1;
		goto cleanup;
	}

	*len = EVP_MD_size(algorithm);
	if (!(*out = malloc(EVP_MD_size(algorithm)))){
		log_enomem();
		ret = -1;
		goto cleanup;
	}

	while ((length = read_file(fp, buffer, sizeof(buffer))) > 0){
		if (ferror(fp)){
			log_efread(file);
			goto cleanup;
		}
		if (EVP_DigestUpdate(ctx, buffer, length) != 1){
			log_error("Failed to calculate checksum");
			ERR_print_errors_fp(stderr);
			goto cleanup;
		}
	}

	if (EVP_DigestFinal_ex(ctx, *out, len) != 1){
		log_error("Failed to finalize checksum calculation");
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

int checksum_bytestring(const char* file, const EVP_MD* algorithm, char** out){
	unsigned char* bytes;
	unsigned len;
	char* buf;

	return_ifnull(out, -1);

	if (checksum(file, algorithm, &bytes, &len) != 0){
		log_error("Failed to calculate checksum for file %s", file);
		return -1;
	}

	if (to_base16(bytes, len, &buf) != 0){
		log_error("Failed to convert checksum bytes to string");
		free(bytes);
		return -1;
	}

	free(bytes);
	*out = buf;
	return 0;
}

int file_to_element(const char* file, const EVP_MD* algorithm, struct element** out){
	unsigned char* buffer = NULL;
	unsigned len = 0;
	int ret = 0;

	return_ifnull(file, -1);
	return_ifnull(out, -1);

	*out = malloc(sizeof(**out));
	if (!out){
		log_enomem();
		return -1;
	}
	(*out)->file = malloc(strlen(file) + 1);
	(*out)->checksum = NULL;
	if (!(*out)->file){
		log_enomem();
		ret = -1;
		goto cleanup;
	}
	strcpy((*out)->file, file);

	/* compute checksum */
	if (checksum(file, algorithm, &buffer, &len) != 0){
		log_debug("checksum() in file_to_element() did not return 0");
		ret = -1;
		goto cleanup;
	}

	/* convert it to hex */
	if (to_base16(buffer, len, &(*out)->checksum) != 0){
		log_error("Failed to convert raw checksum to hexadecimal");
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
int add_checksum_to_file(const char* file, const EVP_MD* algorithm, FILE* out, FILE* prev_checksums, char** out_hash){
	struct element* e;
	char* checksum = NULL;
	int ret;

	return_ifnull(file, -1);
	return_ifnull(out, -1);

	if (out_hash){
		*out_hash = NULL;
	}

	if (!file_opened_for_writing(out)){
		log_emode();
		return -1;
	}

	if (prev_checksums && !file_opened_for_reading(prev_checksums)){
		log_emode();
		return -1;
	}

	if (file_to_element(file, algorithm, &e) != 0){
		log_debug("Could not create element from file");
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

	if (write_element_to_file(out, e) != 0){
		free_element(e);
		free(checksum);
		log_debug("Could not write element to file");
		return -1;
	}

	if (out_hash){
		*out_hash = sh_dup(e->checksum);
		if (!(*out_hash)){
			log_warning("Failed to output hash.");
		}
	}
	free_element(e);
	free(checksum);
	return ret;
}

int sort_checksum_file(const char* in_out){
	struct TMPFILE** tmp_files = NULL;
	struct TMPFILE* tmp_in = NULL;
	size_t n_files = 0;
	FILE* fp_out = NULL;
	size_t i;
	int ret = 0;

	return_ifnull(in_out, -1);

	tmp_in = temp_fopen();
	if (rename_file(in_out, tmp_in->name) != 0){
		log_error("Failed to move checksum file to temp location");
		ret = -1;
		goto cleanup;
	}
	temp_fflush(tmp_in);

	fp_out = fopen(in_out, "wb");
	if (!fp_out){
		log_efopen(in_out);
		ret = -1;
		goto cleanup;
	}

	if (create_initial_runs(tmp_in->fp, &tmp_files, &n_files) != 0){
		log_debug("Error creating initial runs");
		ret = -1;
		goto cleanup;
	}
	if (merge_files(tmp_files, n_files, fp_out) != 0){
		log_debug("Error merging files");
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (ret != 0 && tmp_in){
		rename_file(tmp_in->name, in_out);
	}
	tmp_in ? temp_fclose(tmp_in) : (void)0;
	for (i = 0; i < n_files; ++i){
		temp_fclose(tmp_files[i]);
	}
	free(tmp_files);
	fp_out ? fclose(fp_out) : 0;
	return ret;
}

int search_for_checksum(FILE* fp_checksums, const char* key, char** checksum){
	return_ifnull(fp_checksums, -1);
	return_ifnull(key, -1);
	return_ifnull(checksum, -1);

	if (!file_opened_for_reading(fp_checksums)){
		log_emode();
		return -1;
	}

	return search_file(fp_checksums, key, checksum);
}

int check_file_exists(const char* file){
	struct stat st;

	return_ifnull(file, -1);

	if (lstat(file, &st) == 0){
		return 1;
	}
	switch (errno){
	case ENOENT:
	case ENOTDIR:
		return 0;
	default:
		log_estat(file);
		return -1;
	}
}

int create_removed_list(const char* checksum_file, const char* out_file){
	FILE* fp_checksum = NULL;
	FILE* fp_out = NULL;
	struct element* tmp;
	int err;
	int ret = 0;

	return_ifnull(checksum_file, -1);
	return_ifnull(out_file, -1);

	fp_checksum = fopen(checksum_file, "rb");
	if (!fp_checksum){
		log_efopen(checksum_file);
		ret = -1;
		goto cleanup;
	}
	fp_out = fopen(out_file, "wb");
	if (!fp_out){
		log_efopen(out_file);
		ret = -1;
		goto cleanup;
	}

	while ((tmp = get_next_checksum_element(fp_checksum)) != NULL){
		err = check_file_exists(tmp->file);
		switch (err){
		case 1:
			break;
		case 0:
			fprintf(fp_out, "%s%c\n", tmp->file, '\0');
			break;
		default:
			free_element(tmp);
			return err;
		}
		free_element(tmp);
	}

cleanup:
	if (fp_checksum && fclose(fp_checksum) != 0){
		log_efclose(checksum_file);
	}
	if (fp_out && fclose(fp_out) != 0){
		log_efclose(fp_out);
	}
	return ret;
}

char* get_next_removed(FILE* fp){
	long pos_origin;
	long pos_file;
	int c;
	size_t len_file;
	char* ret = NULL;

	return_ifnull(fp, NULL);

	pos_origin = ftell(fp);

	while ((c = fgetc(fp)) != '\0'){
		if (c == EOF){
			log_debug("get_next_removed(): reached EOF");
			return NULL;
		}
	}
	pos_file = ftell(fp);
	len_file = pos_file - pos_origin;

	ret = malloc(len_file);
	if (!ret){
		log_enomem();
		return NULL;
	}

	fseek(fp, pos_origin, SEEK_SET);
	if (fread(ret, 1, len_file, fp) != len_file){
		log_efread("file");
		free(ret);
		return NULL;
	}
	/* advance file pointer beyond '\n' */
	fgetc(fp);

	return ret;
}
