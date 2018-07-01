/* zip_file.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __COMPRESSION_ZIPFILE_H
#define __COMPRESSION_ZIPFILE_H

#ifndef __ZIP_INTERNAL
#error "Include zip.h, not zip_file.h"
#endif

#include "zip.h"
#include <zlib.h>
#include <bzlib.h>
#include <lzma.h>
#include <stdio.h>

struct ZIP_FILE{
	FILE* fp;
	unsigned write;
	enum COMPRESSOR c_type;
	union tag_strm{
		z_stream zstrm;
		bz_stream bzstrm;
		lzma_stream xzstrm;
	}strm;
};

#endif
