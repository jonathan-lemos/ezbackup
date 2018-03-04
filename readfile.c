#include "readfile.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

/* reads length bytes from fp */
/* returns the number of bytes sucessfully read */
int read_file(FILE* fp, unsigned char* dest, size_t length){
	if (!fp){
		return -1;
	}
	return fread(dest, 1, length, fp);
}

int temp_file(char* template){
	int fd;

	fd = mkstemp(template);
	if (fd < 0){
		return errno;
	}

	close(fd);
	return 0;
}
