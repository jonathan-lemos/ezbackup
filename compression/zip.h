/* zip.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __COMPRESSION_ZIP_H
#define __COMPRESSION_ZIP_H

enum COMPRESSOR{
	COMPRESSOR_INVALID = 0,
#ifndef NO_GZIP_SUPPORT
	COMPRESSOR_GZIP,
#endif
#ifndef NO_BZIP2_SUPPORT
	COMPRESSOR_BZIP2,
#endif
#ifndef NO_XZ_SUPPORT
	COMPRESSOR_XZ,
#endif
#ifndef NO_LZ4_SUPPORT
	COMPRESSOR_LZ4,
#endif
	COMPRESSOR_NONE
};

#define GZIP_NORMAL (0x0)
#define GZIP_HUFFMAN_ONLY (0x1)
#define GZIP_FILTERED (0x2)
#define GZIP_RLE (0x4)
#define GZIP_LOWMEM (0x8)

#define BZIP2_NORMAL (0x0)

#define XZ_NORMAL (0x0)
#define XZ_EXTREME (0x1)

#define LZ4_NORMAL (0x0)

int zip_compress(const char* infile, const char* outfile, enum COMPRESSOR c_type, int compression_level, unsigned flags);
int zip_decompress(const char* infile, const char* outfile, enum COMPRESSOR c_type, unsigned flags);

#endif
