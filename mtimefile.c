/** @file mtimefile.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#define MTIME_BUFFER_LEN (128)

struct entry{
	char* file;
	time_t time;
};

struct data_buffer{
	unsigned char* data;
	size_t len;
};

static int get_file_mtime(const char* file, time_t* out){
	struct stat st;
	if (lstat(file, &st) != 0){
		log_estat(file);
		return -1;
	}
	*out = st.st_ctime;
	return 0;
}

static int read_entry(FILE* fp, struct entry** out){
	struct entry* ret = NULL;
	struct data_buffer buf;
	size_t len;
	size_t len_str;
	unsigned char* zero_ptr = NULL;
	long pos = ftell(fp);
	int c;

	do{
		len = fread(buf, 1, sizeof(buf), fp);
		zero_ptr = memchr(buf, '\0', sizeof(buf));
		/* if len == 0, we reached EOF or read error.
		 * if zero_ptr, we found the location of the zero */
	}while (len > 0 && !zero_ptr);

	/* end of file */
	if (!zero_ptr){
		*out = NULL;
		return 1;
	}

	ret = malloc(sizeof(*ret));
	if (!ret){
		log_enomem();
		*out = NULL;
		return -1;
	}

	len_str =
	ret->file = malloc(
}

static int create_initial_runs(FILE* fp_in, struct TMPFILE*** out, size_t* n_out){

}

int add_mtime_to_file(const char* file, FILE* fp_out, FILE* fp_prev){
	time_t tm;

	if (get_file_mtime(file, &tm) != 0){
		log_warning_ex("Failed to get mtime for %s", file);
		return -1;
	}

	if (fwrite(file, 1, strlen(file) + 1, fp_out) != strlen(file) + 1){
		log_warning_ex("Error writing to mtime file %s", file);
		return -1;
	}
}
