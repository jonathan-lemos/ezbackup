#ifndef __CHECKSUM_H
#define __CHECKSUM_H

/* FILE* */
#include <stdio.h>

int checksum(const char* file, const char* algorithm, unsigned char** out, unsigned* len);
int bytes_to_hex(unsigned char* bytes, unsigned len, char** out);
int add_checksum_to_file(const char* file, const char* algorithm, FILE* out);
int sort_checksum_file(const char* in_file, const char* out_file);
int search_for_checksum(const char* file, const char* key, char** checksum);

#ifdef __TESTING__
#endif

#endif
