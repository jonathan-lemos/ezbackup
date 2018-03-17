/* prototypes */
#include "readfile.h"
/* error handling */
#include "error.h"
#include <errno.h>
#include <string.h>
/* FILE* */
#include <stdio.h>
/* close(fd) */
#include <unistd.h>
/* mkstemp(fd) */
#include <stdlib.h>

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

int temp_file(char* __template){
	int fd;

	fd = mkstemp(__template);
	if (fd < 0){
		log_error("Couldn't create temporary file %s (%s)", __template, strerror(errno));
		return -1;
	}
	close(fd);
	return 0;
}
