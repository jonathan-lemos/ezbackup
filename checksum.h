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
#include <openssl/evp.h>

const EVP_MD* get_evp_md(const char* hash_name);
int checksum(const char* file, const EVP_MD* algorithm, unsigned char** out, unsigned* len);
int bytes_to_hex(const unsigned char* bytes, unsigned len, char** out);
int add_checksum_to_file(const char* file, const EVP_MD* algorithm, FILE* out, FILE* prev_checksums);
int sort_checksum_file(const char* in, const char* out);
int search_for_checksum(FILE* fp, const char* key, char** checksum);
int create_removed_list(const char* checksum_file, const char* out_file);
char* get_next_removed(FILE* fp);

#ifdef __UNIT_TESTING__
int file_to_element(const char* file, const EVP_MD* algorithm, element** out);
int check_file_exists(const char* file);
#endif

#endif
