/** @file mtimefile.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "entry.h"
#include "../log.h"
#include "../file/databuffer.h"
#include "../file/filehelper.h"
#include "../strings/stringhelper.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_RUN_LEN (1 << 24)

static int get_file_mtime(const char* file, time_t* out){
	struct stat st;
	if (lstat(file, &st) != 0){
		log_estat(file);
		return -1;
	}
	*out = st.st_ctime;
	return 0;
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
