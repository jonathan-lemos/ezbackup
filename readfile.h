#ifndef __READFILE_H
#define __READFILE_H

#include <stdio.h>

#ifndef BUFFER_LEN
#define BUFFER_LEN (1 << 16)
#endif

int read_file(FILE* fp, unsigned char* dest, size_t length);
FILE* temp_file(const char* __template);
FILE* temp_file_ex(char* __template);
int shred_file(const char* file);

#endif
