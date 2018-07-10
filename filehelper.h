/* filehelper.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __FILEHELPER_H
#define __FILEHELPER_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#ifndef BUFFER_LEN
#define BUFFER_LEN (1 << 16)
#endif

struct TMPFILE{
	FILE* fp;
	char* name;
};

int read_file(FILE* fp, unsigned char* dest, size_t length);
struct TMPFILE* temp_fopen(void);
int temp_fflush(struct TMPFILE* tfp);
void temp_fclose(struct TMPFILE* tfp);
int file_opened_for_reading(FILE* fp);
int file_opened_for_writing(FILE* fp);
uint64_t get_file_size_fp(FILE* fp);
uint64_t get_file_size(const char* file);
int copy_file(const char* _old, const char* _new);
int rename_file(const char* _old, const char* _new);
int directory_exists(const char* path);
int file_exists(const char* path);
int mkdir_recursive(const char* dir);

#endif
