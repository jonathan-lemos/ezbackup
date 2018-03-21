/* checksum.h -- checksum calculation and checksum file management
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CHECKSUM_H
#define __CHECKSUM_H

/* FILE* */
#include <stdio.h>

int checksum(const char* file, const char* algorithm, unsigned char** out, unsigned* len);
int bytes_to_hex(unsigned char* bytes, unsigned len, char** out);
int add_checksum_to_file(const char* file, const char* algorithm, FILE* out, FILE* prev_checksums);
int sort_checksum_file(FILE* fp_in, FILE* fp_out);
int search_for_checksum(FILE* fp, const char* key, char** checksum);
int create_removed_list(FILE* checksum_file, FILE* out_file);
char* get_next_removed(FILE* fp);

#ifdef __TESTING__
#endif

#endif
