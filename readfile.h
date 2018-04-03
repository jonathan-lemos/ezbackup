/* readfile.h -- buffered file reader and temporary file creator
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __READFILE_H
#define __READFILE_H

#include <stdio.h>

#ifndef BUFFER_LEN
#define BUFFER_LEN (1 << 16)
#endif

struct TMPFILE{
	FILE* fp;
	char* name;
};

int read_file(FILE* fp, unsigned char* dest, size_t length);
struct TMPFILE* temp_fopen(const char* __template, const char* modes);
int temp_fclose(struct TMPFILE* tfp);
int shred_file(const char* file);
int file_opened_for_reading(FILE* fp);
int file_opened_for_writing(FILE* fp);

#endif
