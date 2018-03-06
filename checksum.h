#ifndef __CHECKSUM_H
#define __CHECKSUM_H

/* FILE* */
#include <stdio.h>

int add_checksum_to_file(const char* file, const char* algorithm, FILE* out);
int search_checksum_file(FILE* in, const char* file, char** out);

#ifdef __TESTING__
#endif

#endif
