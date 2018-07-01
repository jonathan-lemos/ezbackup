/* zip_lz4.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "zip.h"
#include "../log.h"
#include "../filehelper.h"
#include <lz4.h>
#include <errno.h>
#include <string.h>

static int lz4_compress_write(FILE* fp_in, FILE* fp_out, int compression_level){
	unsigned char inbuf[BUFFER_LEN];
	unsigned char outbuf[BUFFER_LEN];
	int len_in = 0;
	int len_out = 0;
	const int acceleration = (9 - compression_level) * 8;

	do{
		len_in = read_file(fp_in, inbuf, sizeof(inbuf));
		len_out = LZ4_compress_fast((const char*)inbuf, (char*)outbuf, len_in, sizeof(len_out), acceleration);

		fwrite(outbuf, 1, len_out, fp_out);
	}while (len_in > 0 && len_out > 0);

	return fp_out != 0;
}

int lz4_compress(const char* infile, const char* outfile, int compression_level, unsigned flags){
	FILE* fp_in = NULL;
	FILE* fp_out = NULL;
	int ret = 0;

	fp_in = fopen(infile, "rb");
	if (!fp_in){
		log_efopen(infile);
		ret = -1;
		goto cleanup;
	}

	fp_out = fopen(outfile, "wb");
	if (!fp_out){
		log_efopen(outfile);
		ret = -1;
		goto cleanup;
	}

cleanup:
	fp_in ? fclose(fp_in) : 0;
	if (fp_out && fclose(fp_out) != 0){
		log_efclose(fp_out);
		ret = -1;
	}
	return ret;
}
