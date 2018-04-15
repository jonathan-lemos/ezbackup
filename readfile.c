/* readfile.c -- buffered file reader and temporary file creator
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* prototypes */
#include "readfile.h"
/* seeding the prng */
#include "crypt.h"
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
