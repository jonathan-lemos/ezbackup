/** @file mtimefile/mtimefile.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "mtimefile_sort.h"
#include "../log.h"
#include "../file/databuffer.h"
#include "../file/filehelper.h"
#include "../strings/stringhelper.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

/* for debugging purposes only */
#define MAX_RUN_LEN (1 << 10)

static int get_file_mtime(const char* file, time_t* out){
	struct stat st;
	if (lstat(file, &st) != 0){
		log_estat(file);
		return -1;
	}
	*out = st.st_mtime;
	return 0;
}

int mtime_add_to_file(const char* file, FILE* fp_mtime){
	time_t tm;

	if (get_file_mtime(file, &tm) != 0){
		log_warning_ex("Failed to get mtime for %s", file);
		return -1;
	}

	if (mtime_write(file, tm, fp_mtime) != 0){
		log_warning("Failed to write mtime to file");
		return -1;
	}

	return 0;
}

int mtime_file_changed(const char* file, FILE* fp_mtime_prev){
	time_t tm_file;
	time_t tm_cur;
	int res;

	res = mtime_search(fp_mtime_prev, file, &tm_file);
	if (res < 0){
		log_error("Failed to get mtime from file");
		return -1;
	}
	else if (res > 0){
		log_info_ex("%s: mtime entry not found", file);
		return 1;
	}

	if (get_file_mtime(file, &tm_cur) != 0){
		log_error_ex("Failed to get current mtime for %s", file);
		return -1;
	}

	return tm_file != tm_cur;
}

int mtime_create_removed_list(const char* mtime_file_prev, const char* out_file){
	FILE* fp_out = NULL;
	FILE* fp_mtime = NULL;
	int res;
	int ret = 0;

	fp_out = fopen(out_file, "wb");
	if (!fp_out){
		log_efopen(out_file);
		ret = -1;
		goto cleanup;
	}

	fp_mtime = fopen(mtime_file_prev, "rb");
	if (!fp_mtime){
		log_efopen(mtime_file_prev);
		ret = -1;
		goto cleanup;
	}

	do{
		char* file;
		time_t mtime;
		res = mtime_read(fp_mtime, &file, &mtime);
		if (res < 0){
			log_error("mtime_read() failed");
			ret = -1;
			goto cleanup;
		}
		else if (res > 0){
			log_info("mtime_read() EOF");
			break;
		}

		if (fwrite(file, 1, strlen(file) + 1, fp_out) != 0){
			log_error("Failed to filename to removed list");
			return -1;
		}
	}while (res == 0);

cleanup:
	fp_mtime ? fclose(fp_mtime) : 0;
	if (fp_out && fclose(fp_out) != 0){
		log_efclose(out_file);
	}
	return ret;
}

int mtime_get_next_removed(FILE* fp_removed, char** out){
	struct databuffer db = {0};
	int ret = 0;
	int c;

	if (db_fill(&db) != 0){
		log_error("Failed to initialize data buffer");
		ret = -1;
		goto cleanup;
	}

	do{
		c = fgetc(fp_removed);
		if (c == EOF){
			break;
		}

		if (db_concat_char(&db, c) != 0){
			log_error("Failed to concatenate char to data buffer");
			ret = -1;
			goto cleanup;
		}
	}while (c != '\0');

	if (c == EOF){
		log_info("mtime_get_next_removed() reached EOF");
		ret = 1;
		goto cleanup;
	}

cleanup:
	if (ret != 0){
		db_free_contents(&db);
		*out = NULL;
	}
	else{
		*out = (char*)db.data;
	}
	return ret;
}
