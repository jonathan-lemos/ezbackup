/* filehelper.c -- helper functions for files
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* prototypes */
#include "filehelper.h"
/* error handling */
#include "error.h"
#include <errno.h>
#include <string.h>
/* FILE* */
#include <stdio.h>
/* unlink() */
#include <unistd.h>
/* mkstemp(fd) */
#include <stdlib.h>
/* file size */
#include <sys/stat.h>
/* check file mode */
#include <fcntl.h>

#include <sys/file.h>

/* reads length bytes from fp */
/* returns the number of bytes sucessfully read */
int read_file(FILE* fp, unsigned char* dest, size_t length){
	int ret;
	if (!fp){
		return -1;
	}

	ret = fread(dest, 1, length, fp);
	if (ferror(fp)){
		log_efread("file");
	}
	return ret;
}

FILE* temp_fopen(char* __template){
	int fd;
	FILE* fp;

	/* makes temporary file and opens it in one call,
	 * preventing a race condition where another program
	 * could open it in the mean time */
	fd = mkstemp(__template);

	if (fd < 0){
		log_error_ex("Failed to create temporary file (%s)", strerror(errno));
		return NULL;
	}

	fp = fdopen(fd, "w+b");
	if (!fp){
		log_efopen(__template);
		return NULL;
	}

	return fp;
}

int file_opened_for_reading(FILE* fp){
	int fd;
	int flags;
	if (!fp){
		return 0;
	}
	fd = fileno(fp);
	flags = fcntl(fd, F_GETFL);
	return (flags & O_RDWR) || O_RDONLY == 0 ? !(flags & O_WRONLY) : flags & O_RDONLY;
}

int file_opened_for_writing(FILE* fp){
	int fd;
	int flags;
	if (!fp){
		return 0;
	}
	fd = fileno(fp);
	flags = fcntl(fd, F_GETFL);
	return (flags & O_RDWR) || O_WRONLY == 0 ? !(flags & O_RDONLY) : flags & O_WRONLY;
}

off_t get_file_size_fp(FILE* fp){
	int fd;
	struct stat st;

	fd = fileno(fp);

	if (fstat(fd, &st) != 0){
		log_estat("file");
		return -1;
	}
	return st.st_size;
}

off_t get_file_size(const char* file){
	struct stat st;

	if (stat(file, &st) != 0){
		log_estat(file);
		return -1;
	}
	return st.st_size;
}

int copy_file(const char* _old, const char* _new){
	FILE* fp_old;
	FILE* fp_new;
	unsigned char buffer[BUFFER_LEN];
	int len;

	return_ifnull(_old, -1);
	return_ifnull(_new, -1);

	if (strcmp(_old, _new) == 0){
		return 0;
	}

	fp_old = fopen(_old, "rb");
	if (!fp_old){
		log_efopen(_old);
		return -1;
	}

	fp_new = fopen(_new, "wb");
	if (!fp_new){
		log_efopen(_new);
		fclose(fp_old);
		return -1;
	}

	while ((len = read_file(fp_old, buffer, sizeof(buffer))) > 0){
		fwrite(buffer, 1, len, fp_new);
		if (ferror(fp_new)){
			fclose(fp_old);
			fclose(fp_new);
			log_efwrite(_new);
			return -1;
		}
	}

	fclose(fp_old);

	if (fclose(fp_new) != 0){
		log_efclose(_new);
		return -1;
	}

	return 0;
}

int rename_file(const char* _old, const char* _new){
	int res = 0;

	return_ifnull(_old, -1);
	return_ifnull(_new, -1);

	if (strcmp(_old, _new) == 0){
		return 0;
	}

	if (rename(_old, _new) == 0){
		return 0;
	}

	res = copy_file(_old, _new);
	remove(_old);

	return res;
}
