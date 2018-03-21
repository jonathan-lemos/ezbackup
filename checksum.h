/*
 * Checksum calculation and checksum file module
 * Copyright (C) 2018 Jonathan Lemos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
