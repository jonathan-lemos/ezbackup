/* prototypes */
#include "checksum.h"
/* error handling */
#include "evperror.h"
/* read_file() */
#include "readfile.h"
/* FILE* */
#include <stdio.h>
/* digest algorithms */
#include <openssl/evp.h>
/* strstr() */
#include <string.h>

/* debugging purposes */
#include <assert.h>

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
		return ERR_ARGUMENT_NULL;
	}

	/* 2 hex chars = 1 byte */
	/* +1 for null terminator */
	outlen = len * 2 + 1;
	*out = malloc(outlen);
	if (!out){
		return ERR_OUT_OF_MEMORY;
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
		return evp_geterror();
	}

	/* get EVP_MD* from char* */
	if (algorithm){
		OpenSSL_add_all_algorithms();
		if (!(md = EVP_get_digestbyname(algorithm))){
			EVP_MD_CTX_destroy(ctx);
			return evp_geterror();
		}
	}
	/* default to sha1 if NULL algorithm */
	else{
		md = EVP_sha1();
	}

	if (EVP_DigestInit_ex(ctx, md, NULL) != 1){
		EVP_MD_CTX_destroy(ctx);
		return evp_geterror();
	}

	/* both of these must point to valid locations */
	if (!len || !out){
		return ERR_ARGUMENT_NULL;
	}
	*len = EVP_MD_size(md);
	if (!(*out = malloc(EVP_MD_size(md)))){
		EVP_MD_CTX_destroy(ctx);
		return ERR_OUT_OF_MEMORY;
	}

	while ((length = read_file(fp, buffer, sizeof(buffer))) > 0){
		if (EVP_DigestUpdate(ctx, buffer, length) != 1){
			/* must get error now or EVP_MD_CTX_destroy() will overwrite it */
			int err = evp_geterror();

			free(out);
			EVP_MD_CTX_destroy(ctx);
			return err;
		}
	}

	if (EVP_DigestFinal_ex(ctx, *out, len) != 1){
		int err = evp_geterror();

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
		return ERR_ARGUMENT_NULL;
	}

	*out = malloc(sizeof(*out));
	if (!out){
		return ERR_OUT_OF_MEMORY;
	}
	(*out)->file = malloc(strlen(file) + 1);
	if (!(*out)->file){
		return ERR_OUT_OF_MEMORY;
	}
	strcpy((*out)->file, file);

	/* compute checksum */
	if((err = checksum(file, algorithm, &buffer, &len)) != 0){
		return err;
	}

	/* convert it to hex */
	if ((err = bytes_to_hex(buffer, len, &(*out)->checksum)) != 0){
		free(buffer);
		return err;
	}

	return 0;
}

int write_element_to_file(FILE* fp, element* e){
	if (!fp || !e || !e->file || !e->checksum){
		return ERR_ARGUMENT_NULL;
	}

	fprintf(fp, "%s%c%s%c", e->file, '\0', e->checksum, '\0');
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

	free(e);
	return 0;
}

 element* get_next_checksum_element(FILE* fp){
	long pos_origin;
	long pos_file;
	long pos_checksum;
	long pos_temp;
	int c;
	int counter = 0;
	size_t len_file;
	size_t len_checksum;
	element* e;

	e = malloc(sizeof(*e));
	if (!fp || !e){
		return NULL;
	}

	pos_origin = ftell(fp);
	while (counter < 2){
		c = fgetc(fp);
		if (c == EOF){
			break;
		}
		if (c == '\0'){
			counter++;
			if (counter & 1){
				pos_file = ftell(fp);
			}
			else{
				pos_checksum = ftell(fp);
			}
		}
	}
	len_file = pos_file - pos_origin;
	len_checksum = pos_checksum - pos_file;

	e->file = malloc(len_file);
	if (!e->file){
		free(e);
		return NULL;
	}
	e->checksum = malloc(len_checksum);
	if (!e){
		free(e->file);
		free(e);
		return NULL;
	}

	fseek(fp, pos_origin, SEEK_SET);
	fread(e->file, 1, len_file, fp);

	pos_temp = ftell(fp);
	assert(ftell(fp) == pos_temp);

	fread(e->checksum, 1, len_checksum, fp);

	return e;
}

 element* get_checksum_element_index(FILE* fp, int index){
	long pos_origin;
	long pos_file;
	long pos_checksum;
	size_t len_file;
	size_t len_checksum;
	int c;
	int counter = 0;
	element* e;

	e = malloc(sizeof(*e));
	if (!fp || !e){
		return NULL;
	}

	rewind(fp);
	while (counter < 2 * index){
		c = fgetc(fp);
		if (c == EOF){
			break;
		}
		if (c == '\0'){
			counter++;
			if (!(counter & 1)){
				pos_origin = ftell(fp);
			}
		}
	}
	counter = 0;
	while (counter < 2){
		c = fgetc(fp);
		if (c == EOF){
			break;
		}
		if (c == '\0'){
			counter++;
			if (counter & 1){
				pos_file = ftell(fp);
			}
			else{
				pos_checksum = ftell(fp);
			}
		}
	}
	len_file = pos_file - pos_origin;
	len_checksum = pos_checksum - pos_file;

	e->file = malloc(len_file);
	if (!e->file){
		free(e);
		return NULL;
	}
	fseek(fp, pos_origin, SEEK_SET);
	fread(e->file, 1, len_file, fp);

	assert(pos_file == ftell(fp));
	e->checksum = malloc(len_checksum);
	if (!e->checksum){
		free(e->file);
		free(e);
		return NULL;
	}
	fread(e->checksum, 1, len_checksum, fp);
	return e;
}

void swap(element** e1, element** e2){
	element* buf = *e1;
	*e1 = *e2;
	*e2 = buf;
}

 int median_of_three(element** elements, int low, int high){
	int left = low;
	int mid = (high - low) / 2;
	int right = high;

	if (high - low < 3){
		return low;
	}

	if (strcmp(elements[left]->file, elements[mid]->file) > 0){
		if (strcmp(elements[mid]->file, elements[right]->file) > 0){
			return mid;
		}
		else if (strcmp(elements[right]->file, elements[left]->file) > 0){
			return left;
		}
		else{
			return right;
		}
	}
	else{
		if (strcmp(elements[mid]->file, elements[right]->file) < 0){
			return mid;
		}
		else if (strcmp(elements[right]->file, elements[left]->file) < 0){
			return left;
		}
		else{
			return right;
		}
	}
}


static int partition(element** elements, int low, int high){
	element* pivot = elements[high];
	int i;
	int j;

	for (i = low - 1, j = low; j <= high - 1; ++j){
		if (strcmp(elements[j]->file, pivot->file) < 0){
			i++;
			swap(&elements[i], &elements[j]);
		}
	}
	swap(&elements[i + 1], &elements[high]);
	return i + 1;
}

static int partition_m3(element** elements, int low, int high){
	int m3 = median_of_three(elements, low, high);
	swap(&elements[m3], &elements[high]);
	return partition(elements, low, high);
}

void quicksort_elements(element** elements, int low, int high){
	if (low < high){
		int pivot = partition_m3(elements, low, high);
		quicksort_elements(elements, low, pivot - 1);
		quicksort_elements(elements, pivot + 1, high);
	}
}

void free_elements(element** elements, size_t size){
	size_t i;
	for (i = 0; i < size; ++i){
		free(elements[i]->file);
		free(elements[i]->checksum);
	}
	free(elements);
}

int sort_checksum_file(const char* file){
	return 0;
}
