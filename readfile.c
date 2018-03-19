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

/* reads length bytes from fp */
/* returns the number of bytes sucessfully read */
int read_file(FILE* fp, unsigned char* dest, size_t length){
	int ret;
	if (!fp){
		return -1;
	}

	ret = fread(dest, 1, length, fp);
	if (ferror(fp)){
		log_error("Error reading file (%s)", strerror(errno));
	}
	return ret;
}

FILE* temp_file(const char* __template){
	char* tmp;
	int fd;
	FILE* fp;

	/* template must be writable */
	tmp = malloc(strlen(__template) + 1);
	if (!tmp){
		log_fatal(STR_ENOMEM);
		return NULL;
	}
	strcpy(tmp, __template);

	/* makes temporary file and opens it in one call,
	 * preventing a race condition where another program
	 * could open it in the mean time */
	fd = mkstemp(tmp);
	/* deletes temporary file as soon as fclose() is called
	 * on it */
	unlink(tmp);

	if (fd < 0){
		log_error("Couldn't create temporary file %s (%s)", __template, strerror(errno));
		free(tmp);
		return NULL;
	}

	fp = fdopen(fd, "w+b");
	if (!fp){
		log_error("Failed to open temporary file %s (%s)", tmp, strerror(errno));
		free(tmp);
		return NULL;
	}

	free(tmp);
	return fp;
}

FILE* temp_file_ex(char* __template){
	int fd;
	FILE* fp;

	fd = mkstemp(__template);

	if (fd < 0){
		log_error("Couldn't create temporary file %s (%s)", __template, strerror(errno));
		return NULL;
	}

	fp = fdopen(fd, "w+b");
	if (!fp){
		log_error("Failed to open temporary file %s (%s)", __template, strerror(errno));
		return NULL;
	}

	return fp;
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
			log_warning("Could not open %s for shredding (%s)", file, strerror(errno));
			fclose(fp);
			return -1;
		}

		if (stat(file, &st) != 0){
			log_warning("Could not stat %s for shredding (%s)", file, strerror(errno));
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
