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

/* reads length bytes from fp */
/* returns the number of bytes sucessfully read */
int read_file(FILE* fp, unsigned char* dest, size_t length){
	int ret;
	if (!fp){
		return -1;
	}

	ret = fread(dest, 1, length, fp);
	if (ferror(fp)){
		log_error(__FL__, "Error reading file (%s)", strerror(errno));
	}
	return ret;
}

TMPFILE* temp_fopen(const char* __template){
	int fd;
	TMPFILE* tfp;

	tfp = malloc(sizeof(*tfp));
	if (!tfp){
		log_fatal(__FL__, STR_ENOMEM);
		return NULL;
	}

	if (__template){
		/* template must be writable */
		tfp->name = malloc(strlen(__template) + 1);
		if (!(tfp->name)){
			log_fatal(__FL__, STR_ENOMEM);
			free(tfp);
			return NULL;
		}
		strcpy(tfp->name, __template);
	}
	else{
		tfp->name = malloc(sizeof("/tmp/tmpfile_XXXXXX"));
		if (!(tfp->name)){
			log_fatal(__FL__, STR_ENOMEM);
			free(tfp);
			return NULL;
		}
		strcpy(tfp->name, "/tmp/tmpfile_XXXXXX");
	}

	/* makes temporary file and opens it in one call,
	 * preventing a race condition where another program
	 * could open it in the mean time */
	fd = mkstemp(tfp->name);

	if (fd < 0){
		log_error(__FL__, "Couldn't create temporary file %s (%s)", __template, strerror(errno));
		free(tfp->name);
		free(tfp);
		return NULL;
	}

	tfp->fp = fdopen(fd, "w+b");
	if (!(tfp->fp)){
		log_error(__FL__, "Failed to open temporary file %s (%s)", tfp->name, strerror(errno));
		free(tfp->name);
		free(tfp);
		return NULL;
	}

	return tfp;
}

int temp_fclose(TMPFILE* tfp){
	int ret = 0;

	if (!tfp){
		return -1;
	}

	if (fclose(tfp->fp) != 0){
		ret = -1;
		log_warning(__FL__, STR_EFCLOSE, tfp->name);
	}
	remove(tfp->name);
	free(tfp->name);
	free(tfp);
	return ret;
}

int shred_file(const char* file){
	struct stat st;
	FILE* fp;
	__off_t size;
	__off_t i;
	int j;
	unsigned k;
	/* don't want compiler optimizing this */
	volatile unsigned seed = 0;

	/* generate truly random seed */
	for (k = 0; k < sizeof(unsigned); ++k){
		seed += crypt_randc() << (8 * k);
	}
	srandom(seed);
	/* get seed out of memory */
	seed = 0;

	/* wipe file 3 times with truly random data */
	for (j = 0; j < 3; ++j){
		fp = fopen(file, "wb");
		if (!fp){
			log_warning(__FL__, "Could not open %s for shredding (%s)", file, strerror(errno));
			fclose(fp);
			return -1;
		}

		if (stat(file, &st) != 0){
			log_warning(__FL__, "Could not stat %s for shredding (%s)", file, strerror(errno));
			fclose(fp);
			return -1;
		}

		size = st.st_size;
		for (i = 0; i < size; ++i){
			fputc(random(), fp);
		}
		fclose(fp);
	}

	remove(file);
	return 0;
}

int file_opened_for_reading(FILE* fp){
	int fd;
	if (!fp){
		return 0;
	}
	fd = fileno(fp);
	return (fcntl(fd, F_GETFL) & O_RDONLY) || (fcntl(fd, F_GETFL) & O_RDWR);
}

int file_opened_for_writing(FILE* fp){
	int fd;
	if (!fp){
		return 0;
	}
	fd = fileno(fp);
	return (fcntl(fd, F_GETFL) & O_WRONLY) || (fcntl(fd, F_GETFL) & O_RDWR);
}
