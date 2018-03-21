/*
 * TAR + compression module
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

#ifndef __MAKETAR_H
#define __MAKETAR_H

#include <stdio.h>

typedef struct archive TAR;
typedef enum COMPRESSOR { COMPRESSOR_NONE, COMPRESSOR_LZ4, COMPRESSOR_GZIP, COMPRESSOR_BZIP2, COMPRESSOR_XZ, COMPRESSOR_INVALID } COMPRESSOR;

TAR* tar_create(const char* filename, COMPRESSOR comp);
int tar_add_file(TAR* tp, const char* filename);
int tar_add_file_ex(TAR* tp, const char* filename, const char* path_in_tar, int verbose, const char* progress_msg);
int tar_add_fp_ex(TAR* tp, FILE* fp, const char* path_in_tar, int verbose, const char* progress_msg);
int tar_close(TAR* fp);
int tar_extract(const char* tarchive, const char* outdir);
int tar_extract_file(const char* tarchive, const char* file_intar, const char* out);
COMPRESSOR get_compressor_byname(const char* compressor);
const char* compressor_to_string(COMPRESSOR comp);

#endif
