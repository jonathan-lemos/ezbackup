/* zip_lz4.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __COMPRESSION_ZIP_LZ4_H
#define __COMPRESSION_ZIP_LZ4_H

int lz4_compress(const char* infile, const char* outfile, int compression_level, unsigned flags);
int lz4_decompress(const char* infile, const char* outfile, unsigned flags);

#endif
