/* zip_lz4.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef NO_LZ4_SUPPORT

#include "zip_lz4.h"
#include "zip.h"
#include "../log.h"
#include "../filehelper.h"
#include <lz4.h>
#include <lz4hc.h>
#include <lz4frame.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static int lz4_compress_write(FILE* fp_in, FILE* fp_out, int compression_level, LZ4F_compressionContext_t ctx){
	unsigned char inbuf[BUFFER_LEN];
	unsigned char outbuf[BUFFER_LEN];
	int len_in = 0;
	int len_out = 0;
	size_t len;
	LZ4F_preferences_t prefs = {
		{ LZ4F_max256KB, LZ4F_blockLinked, LZ4F_noContentChecksum, LZ4F_frame,
			0, 0, LZ4F_noBlockChecksum},
		0, /* compression level. must be a compile time constant, so we can't set it to compression_level */
		0, /* autoflush = 0 */
		0, /* favor decompression speed = 0 */
		{0, 0, 0} /* reserved */
	};
	prefs.compressionLevel = compression_level + 3;

	len = LZ4F_compressBegin(ctx, outbuf, sizeof(outbuf), &prefs);
	if (LZ4F_isError(len)){
		log_error("Failed to write LZ4 header");
		return -1;
	}
	if (fwrite(outbuf, 1, len, fp_out) != len){
		log_efwrite("lz4 output");
		return -1;
	}

	do{
		len_in = fread(inbuf, 1, sizeof(inbuf), fp_in);
		if (len_in < 0){
			log_efread("lz4 input");
			return -1;
		}
		else if (len_in == 0){
			break;
		}

		len_out = LZ4F_compressUpdate(ctx, outbuf, sizeof(outbuf), inbuf, len_in, NULL);
		if (LZ4F_isError(len_out)){
			log_error("LZ4 compression error");
			return -1;
		}

		if ((int)fwrite(outbuf, 1, len_out, fp_out) != len_out){
			log_efwrite("lz4 output");
			return -1;
		}
	}while (len_out > 0);

	len_out = LZ4F_compressEnd(ctx, outbuf, sizeof(outbuf), NULL);
	if (LZ4F_isError(len_out)){
		log_error("Failed to finish lz4 output");
		return -1;
	}

	if ((int)fwrite(ctx, 1, len_out, fp_out) != len_out){
		log_efwrite("lz4 output");
		return -1;
	}

	return 0;
}

int lz4_compress(const char* infile, const char* outfile, int compression_level, unsigned flags){
	FILE* fp_in = NULL;
	FILE* fp_out = NULL;
	LZ4F_compressionContext_t ctx = NULL;
	size_t err;
	int ret = 0;

	(void)flags;

	err = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
	if (LZ4F_isError(err)){
		log_error("Failed to create LZ4 compression context");
		ret = -1;
		goto cleanup;
	}

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

	if (lz4_compress_write(fp_in, fp_out, compression_level, ctx) != 0){
		log_error("Error compressing file");
		ret = -1;
		goto cleanup;
	}

cleanup:
	LZ4F_freeCompressionContext(ctx);
	fp_in ? fclose(fp_in) : 0;
	if (fp_out && fclose(fp_out) != 0){
		log_efclose(fp_out);
		ret = -1;
	}
	return ret;
}

static int lz4_decompress_internal(FILE* fp_in, FILE* fp_out, size_t block_size, size_t initial_read_len, size_t header_len, LZ4F_dctx* dctx){
	unsigned char inbuf[BUFFER_LEN];
	unsigned char* in_ptr = inbuf;
	unsigned char* in_end = inbuf + sizeof(inbuf);
	unsigned char* outbuf = NULL;
	size_t in_len = 0;
	int first_chunk = 1;

	outbuf = malloc(block_size);
	if (!outbuf){
		log_enomem();
		return -1;
	}

	do{
		in_len = first_chunk ? initial_read_len : fread(inbuf, 1, sizeof(inbuf), fp_in);
		if (in_len == 0){
			continue;
		}

		first_chunk = 0;

		in_ptr = inbuf + header_len;
		header_len = 0;

		while (in_ptr < in_end){
			size_t out_size = block_size;
			size_t in_ptr_len = in_end - in_ptr;
			size_t res = LZ4F_decompress(dctx, outbuf, &out_size, in_ptr, &in_ptr_len, NULL);
			if (LZ4F_isError(res)){
				log_error("LZ4 decompression error");
				free(outbuf);
				return -1;
			}

			if (fwrite(outbuf, 1, out_size, fp_out) != 0){
				log_efwrite("output file");
				free(outbuf);
				return -1;
			}

			in_ptr += in_ptr_len;
		}
	}while (in_len > 0);

	free(outbuf);
	return 0;
}

static int lz4_decompress_read(FILE* fp_in, FILE* fp_out, LZ4F_dctx* dctx){
	unsigned char inbuf[LZ4F_HEADER_SIZE_MAX];
	size_t block_size = 0;
	size_t in_len = 0;
	size_t header_len = 0;
	size_t len;
	LZ4F_frameInfo_t info;

	in_len = fread(inbuf, 1, sizeof(inbuf), fp_in);
	if (in_len == 0){
		log_efread("lz4 compressed file");
		return -1;
	}

	header_len = in_len;
	len = LZ4F_getFrameInfo(dctx, &info, inbuf, &header_len);
	if (LZ4F_isError(len)){
		log_error("Failed to read LZ4 frame info (possibly corrupted file)");
		return -1;
	}

	switch (info.blockSizeID){
	case LZ4F_default:
	case LZ4F_max64KB:
		block_size = 1 << 16;
		break;
	case LZ4F_max256KB:
		block_size = 1 << 18;
		break;
	case LZ4F_max1MB:
		block_size = 1 << 20;
		break;
	case LZ4F_max4MB:
		block_size = 1 << 22;
		break;
	default:
		log_error("Invalid LZ4 block size");
		return -1;
	}

	if (lz4_decompress_internal(fp_in, fp_out, block_size, in_len, header_len, dctx) != 0){
		log_error("Failed to decompress data");
		return -1;
	}

	return 0;
}

int lz4_decompress(const char* infile, const char* outfile, unsigned flags){
	FILE* fp_in = NULL;
	FILE* fp_out = NULL;
	LZ4F_dctx* dctx = NULL;
	size_t err;
	int ret = 0;

	(void)flags;

	err = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
	if (LZ4F_isError(err)){
		log_error("Failed to create LZ4 compression context");
		ret = -1;
		goto cleanup;
	}

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

	if (lz4_decompress_read(fp_in, fp_out, dctx) != 0){
		log_error("Error compressing file");
		ret = -1;
		goto cleanup;
	}

cleanup:
	LZ4F_freeDecompressionContext(dctx);
	fp_in ? fclose(fp_in) : 0;
	if (fp_out && fclose(fp_out) != 0){
		log_efclose(fp_out);
		ret = -1;
	}
	return ret;
}

#endif
