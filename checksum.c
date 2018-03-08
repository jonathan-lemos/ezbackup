/* prototypes */
#include "checksum.h"
/* error handling */
#include "error.h"
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
/* file size */
#include <sys/stat.h>

/* debugging purposes */
#include <assert.h>

/* 16mb */
#define MAX_RUN_SIZE (1 << 24)

static int bytes_to_hex(unsigned char* bytes, unsigned len, char** out){
	unsigned i;
	unsigned outptr;
	unsigned outlen;
	const char hexmap[] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F'};

	if (!out || !bytes){
		return err_regularerror(ERR_ARGUMENT_NULL);
	}

	/* 2 hex chars = 1 byte */
	/* +1 for null terminator */
	outlen = len * 2 + 1;
	*out = malloc(outlen);
	if (!out){
		return err_regularerror(ERR_OUT_OF_MEMORY);
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
	EVP_MD_CTX* ctx;
	const EVP_MD* md;
	unsigned char buffer[BUFFER_LEN];
	int length;
	FILE* fp;

	fp = fopen(file, "rb");
	if (!fp){
		return ERR_FILE_INPUT;
	}

	if (!(ctx = EVP_MD_CTX_create())){
		return err_evperror();
	}

	/* get EVP_MD* from char* */
	if (algorithm){
		OpenSSL_add_all_algorithms();
		if (!(md = EVP_get_digestbyname(algorithm))){
			EVP_MD_CTX_destroy(ctx);
			return err_evperror();
		}
	}
	/* default to sha1 if NULL algorithm */
	else{
		md = EVP_sha1();
	}

	if (EVP_DigestInit_ex(ctx, md, NULL) != 1){
		EVP_MD_CTX_destroy(ctx);
		return err_evperror();
	}

	/* both of these must point to valid locations */
	if (!len || !out){
		return err_regularerror(ERR_ARGUMENT_NULL);
	}
	*len = EVP_MD_size(md);
	if (!(*out = malloc(EVP_MD_size(md)))){
		EVP_MD_CTX_destroy(ctx);
		return err_regularerror(ERR_OUT_OF_MEMORY);
	}

	while ((length = read_file(fp, buffer, sizeof(buffer))) > 0){
		if (EVP_DigestUpdate(ctx, buffer, length) != 1){
			/* must get error now or EVP_MD_CTX_destroy() will overwrite it */
			int err = err_evperror();

			free(out);
			EVP_MD_CTX_destroy(ctx);
			return err;
		}
	}

	if (EVP_DigestFinal_ex(ctx, *out, len) != 1){
		int err = err_evperror();

		free(out);
		EVP_MD_CTX_destroy(ctx);
		return err;
	}

	EVP_MD_CTX_destroy(ctx);
	return 0;
}

int file_to_element(const char* file, const char* algorithm, element** out){
	unsigned char* buffer;
	unsigned len;
	int err;

	if (!file || !out){
		return err_regularerror(ERR_ARGUMENT_NULL);
	}

	*out = malloc(sizeof(**out));
	if (!out){
		return err_regularerror(ERR_OUT_OF_MEMORY);
	}
	(*out)->file = malloc(strlen(file) + 1);
	if (!(*out)->file){
		return err_regularerror(ERR_OUT_OF_MEMORY);
	}
	strcpy((*out)->file, file);

	/* compute checksum */
	if ((err = checksum(file, algorithm, &buffer, &len)) != 0){
		return err;
	}

	/* convert it to hex */
	if ((err = bytes_to_hex(buffer, len, &(*out)->checksum)) != 0){
		free(buffer);
		return err;
	}

	free(buffer);
	return 0;
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
int add_checksum_to_file(const char* file, const char* algorithm, FILE* out){
	element* e;
	int err;

	if ((err = file_to_element(file, algorithm, &e)) != 0){
		return err;
	}

	if ((err = write_element_to_file(out, e)) != 0){
		return err;
	}

	free_element(e);
	return 0;
}

int sort_checksum_file(const char* in_file, const char* out_file){
	char** tmp_files;
	size_t n_files;
	size_t i;

	create_initial_runs(in_file, &tmp_files, &n_files);
	merge_files(tmp_files, n_files, out_file);
	for (i = 0; i < n_files; ++i){
		free(tmp_files[i]);
	}
	free(tmp_files);
	return 0;
}


