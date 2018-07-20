/** @file compression/zip_lz4.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __COMPRESSION_ZIP_LZ4_H
#define __COMPRESSION_ZIP_LZ4_H

#ifndef __ZIP_INTERNAL
#error "Include zip.h, not zip_lz4.h"
#endif

#include "zip.h"

int lz4_compress(const char* infile, const char* outfile, int compression_level, enum lz4_flags flags);
int lz4_decompress(const char* infile, const char* outfile, enum lz4_flags flags);

#endif
