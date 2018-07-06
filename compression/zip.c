/* zip.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#define __ZIP_INTERNAL
#include "zip.h"
#include "zip_file.h"
#include "../log.h"
#define BUFFER_LEN (32)
#include "../filehelper.h"
#include "../strings/stringhelper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef NO_GZIP_SUPPORT
#include <zlib.h>
#endif
#ifndef NO_BZIP2_SUPPORT
#include <bzlib.h>
#endif
#ifndef NO_XZ_SUPPORT
#include <lzma.h>
#endif
#ifndef NO_LZ4_SUPPORT
#include "zip_lz4.h"
#endif

static void* zalloc(void* q, unsigned n, unsigned m){
	(void)q;
	return calloc(n, m);
}

static void zfree(void* q, void* p){
	(void)q;
	free(p);
}

#ifndef NO_GZIP_SUPPORT
static struct ZIP_FILE* gzip_open(const char* file, const char* mode){
	struct ZIP_FILE* ret = NULL;
	const int gz_windowbits = 15 + 16;

	ret = malloc(sizeof(*ret));
	if (!ret){
		log_enomem();
		return NULL;
	}

	ret->c_type = COMPRESSOR_GZIP;
	ret->strm.zstrm.zalloc = zalloc;
	ret->strm.zstrm.zfree = zfree;
	ret->strm.zstrm.opaque = Z_NULL;
	ret->write = strchr(mode, 'w') != NULL;

	if (ret->write){
		int compression_level = -1;
		int i;
		int strategy = Z_DEFAULT_STRATEGY;
		if (strchr(mode, 'f') != NULL){
			strategy = Z_FILTERED;
		}
		else if (strchr(mode, 'h') != NULL){
			strategy = Z_HUFFMAN_ONLY;
		}
		else if (strchr(mode, 'R') != NULL){
			strategy = Z_RLE;
		}

		for (i = 0; i <= 9; ++i){
			if (strchr(mode, i + '0') != NULL){
				compression_level = i;
				break;
			}
		}

		ret->fp = fopen(file, "wb");
		if (!ret->fp){
			log_efopen(file);
			free(ret);
			return NULL;
		}

		if (deflateInit2(&(ret->strm.zstrm), compression_level, Z_DEFLATED, gz_windowbits, 8, strategy) != 0){
			log_error("Failed to initialize compression operation");
			free(ret);
			return NULL;
		}
	}
	else{
		ret->strm.zstrm.next_in = Z_NULL;
		ret->strm.zstrm.avail_in = 0;

		ret->fp = fopen(file, "rb");
		if (!ret->fp){
			log_efopen(file);
			free(ret);
			return NULL;
		}

		if (inflateInit2(&(ret->strm.zstrm), gz_windowbits) != Z_OK){
			log_error("Failed to initialize decompression operation");
			free(ret);
			return NULL;
		}
	}
	return ret;
}
#endif

#ifndef NO_BZIP2_SUPPORT
static struct ZIP_FILE* bzip2_open(const char* file, const char* mode){
	struct ZIP_FILE* ret = NULL;

	ret = malloc(sizeof(*ret));
	if (!ret){
		log_enomem();
		return NULL;
	}

	ret->c_type = COMPRESSOR_BZIP2;
	ret->strm.bzstrm.bzalloc = NULL;
	ret->strm.bzstrm.bzfree = NULL;
	ret->strm.bzstrm.opaque = NULL;
	ret->write = strchr(mode, 'w') != NULL;

	if (ret->write){
		int compression_level = -1;
		int i;

		for (i = 0; i <= 9; ++i){
			if (strchr(mode, i + '0') != NULL){
				compression_level = i;
				break;
			}
		}

		ret->fp = fopen(file, "wb");
		if (!ret->fp){
			log_efopen(file);
			free(ret);
			return NULL;
		}

		if (BZ2_bzCompressInit(&(ret->strm.bzstrm), compression_level, 0, 30) != 0){
			log_error("Failed to initialize compression operation");
			free(ret);
			return NULL;
		}
	}
	else{
		ret->strm.bzstrm.next_in = NULL;
		ret->strm.zstrm.avail_in = 0;

		ret->fp = fopen(file, "rb");
		if (!ret->fp){
			log_efopen(file);
			free(ret);
			return NULL;
		}

		if (BZ2_bzDecompressInit(&(ret->strm.bzstrm), 0, 0) != BZ_OK){
			log_error("Failed to initialize decompression operation");
			free(ret);
			return NULL;
		}
	}
	return ret;
}
#endif

#ifndef NO_XZ_SUPPORT
static struct ZIP_FILE* xz_open(const char* file, const char* mode){
	struct ZIP_FILE* ret = NULL;
	lzma_stream xstrm = LZMA_STREAM_INIT;

	ret = malloc(sizeof(*ret));
	if (!ret){
		log_enomem();
		return NULL;
	}

	ret->c_type = COMPRESSOR_XZ;
	ret->strm.xzstrm = xstrm;
	ret->write = strchr(mode, 'w') != NULL;

	if (ret->write){
		uint32_t compression_level = -1;
		int i;

		for (i = 0; i <= 9; ++i){
			if (strchr(mode, i + '0') != NULL){
				compression_level = i;
				break;
			}
		}

		if (strchr(mode, 'e') != NULL){
			compression_level |= LZMA_PRESET_EXTREME;
		}

		ret->fp = fopen(file, "wb");
		if (!ret->fp){
			log_efopen(file);
			free(ret);
			return NULL;
		}

		if (lzma_easy_encoder(&(ret->strm.xzstrm), compression_level, LZMA_CHECK_CRC64) != LZMA_OK){
			log_error("Error initializing LZMA compression operation");
			free(ret);
			return NULL;
		}
	}
	else{
		ret->fp = fopen(file, "rb");
		if (!ret->fp){
			log_efopen(file);
			free(ret);
			return NULL;
		}

		if (lzma_stream_decoder(&(ret->strm.xzstrm), UINT64_MAX, 0) != LZMA_OK){
			log_error("Failed to initialize decompression operation");
			free(ret);
			return NULL;
		}
	}
	return ret;
}
#endif

static struct ZIP_FILE* zip_open(const char* file, int write, enum COMPRESSOR c_type, int compression_level, int flags){
	char truemode[7];
	int modeptr = 2;

	if (c_type == COMPRESSOR_NONE){
		return NULL;
	}

	truemode[0] = write ? 'w' : 'r';
	truemode[1] = 'b';
	if (compression_level >= 0 && compression_level <= 9){
		truemode[modeptr] = compression_level + '0';
		modeptr++;
	}
	if (write){
		switch (c_type){
#ifndef NO_GZIP_SUPPORT
		case COMPRESSOR_GZIP:
			if (flags & GZIP_HUFFMAN_ONLY){
				truemode[modeptr] = 'h';
				modeptr++;
			}
			else if (flags & GZIP_FILTERED){
				truemode[modeptr] = 'f';
				modeptr++;
			}
			else if (flags & GZIP_RLE){
				truemode[modeptr] = 'R';
				modeptr++;
			}
			break;
#endif
#ifndef NO_XZ_SUPPORT
		case COMPRESSOR_XZ:
			if (flags & XZ_EXTREME){
				truemode[modeptr] = 'e';
				modeptr++;
			}
#endif
			/* shut up gcc */
		default:
			;
		}
	}
	truemode[modeptr] = '\0';

	switch (c_type){
#ifndef NO_GZIP_SUPPORT
	case COMPRESSOR_GZIP:
		return gzip_open(file, truemode);
#endif
#ifndef NO_BZIP2_SUPPORT
	case COMPRESSOR_BZIP2:
		return bzip2_open(file, truemode);
#endif
#ifndef NO_XZ_SUPPORT
	case COMPRESSOR_XZ:
		return xz_open(file, truemode);
#endif
	default:
		log_error("not supported");
		return NULL;
	}
}

static int zip_close(struct ZIP_FILE* zfp){
	if (!zfp){
		return -1;
	}

	if (zfp->write){
		switch (zfp->c_type){
#ifndef NO_GZIP_SUPPORT
		case COMPRESSOR_GZIP:
			deflateEnd(&(zfp->strm.zstrm));
			break;
#endif
#ifndef NO_BZIP2_SUPPORT
		case COMPRESSOR_BZIP2:
			BZ2_bzCompressEnd(&(zfp->strm.bzstrm));
			break;
#endif
#ifndef NO_XZ_SUPPORT
		case COMPRESSOR_XZ:
			lzma_end(&(zfp->strm.xzstrm));
			break;
#endif
		default:
			log_fatal("not supported");
		}
	}
	else{
		switch (zfp->c_type){
#ifndef NO_GZIP_SUPPORT
		case COMPRESSOR_GZIP:
			inflateEnd(&(zfp->strm.zstrm));
			break;
#endif
#ifndef NO_BZIP2_SUPPORT
		case COMPRESSOR_BZIP2:
			BZ2_bzDecompressEnd(&(zfp->strm.bzstrm));
			break;
#endif
#ifndef NO_XZ_SUPPORT
		case COMPRESSOR_XZ:
			lzma_end(&(zfp->strm.xzstrm));
			break;
#endif
		default:
			log_fatal("not supported");
		}
	}

	if (fclose(zfp->fp) != 0){
		log_efclose("file");
	}
	free(zfp);
	return 0;
}

static int zip_compress_write(FILE* fp_in, struct ZIP_FILE* zfp){
	unsigned char inbuf[BUFFER_LEN];
	unsigned char outbuf[BUFFER_LEN];
	int avail_in = 0;
	int avail_out = 0;
	int action;

	switch (zfp->c_type){
#ifndef NO_GZIP_SUPPORT
	case COMPRESSOR_GZIP:
		zfp->strm.zstrm.next_in = NULL;
		zfp->strm.zstrm.avail_in = 0;
		zfp->strm.zstrm.next_out = outbuf;
		zfp->strm.zstrm.avail_out = sizeof(outbuf);
		action = Z_NO_FLUSH;
		break;
#endif
#ifndef NO_BZIP2_SUPPORT
	case COMPRESSOR_BZIP2:
		zfp->strm.bzstrm.next_in = NULL;
		zfp->strm.bzstrm.avail_in = 0;
		zfp->strm.bzstrm.next_out = (char*)outbuf;
		zfp->strm.bzstrm.avail_out = sizeof(outbuf);
		action = BZ_RUN;
		break;
#endif
#ifndef NO_XZ_SUPPORT
	case COMPRESSOR_XZ:
		zfp->strm.xzstrm.next_in = NULL;
		zfp->strm.xzstrm.avail_in = 0;
		zfp->strm.xzstrm.next_out = outbuf;
		zfp->strm.xzstrm.avail_out = sizeof(outbuf);
		action = LZMA_RUN;
		break;
#endif
	default:
		log_error("unsupported");
		return -1;
	}

	do{
		size_t write_len;
		int res = 0;

		avail_in = read_file(fp_in, inbuf, sizeof(inbuf));
		if (avail_in == 0 || feof(fp_in)){
			switch (zfp->c_type){
#ifndef NO_GZIP_SUPPORT
			case COMPRESSOR_GZIP:
				action = Z_FINISH;
				break;
#endif
#ifndef NO_BZIP2_SUPPORT
			case COMPRESSOR_BZIP2:
				action = BZ_FINISH;
				break;
#endif
#ifndef NO_XZ_SUPPORT
			case COMPRESSOR_XZ:
				action = LZMA_FINISH;
				break;
#endif
			default:
				;
			}
		}

		switch (zfp->c_type){
#ifndef NO_GZIP_SUPPORT
		case COMPRESSOR_GZIP:
			zfp->strm.zstrm.next_in = inbuf;
			zfp->strm.zstrm.avail_in = avail_in;
			zfp->strm.zstrm.next_out = outbuf;
			zfp->strm.zstrm.avail_out = sizeof(outbuf);
			res = deflate(&(zfp->strm.zstrm), action);
			if (res != Z_OK && res != Z_STREAM_END){
				log_error_ex("gzip write error (%d)", res);
				return -1;
			}
			avail_out = zfp->strm.zstrm.avail_out;
			break;
#endif
#ifndef NO_BZIP2_SUPPORT
		case COMPRESSOR_BZIP2:
			zfp->strm.bzstrm.next_in = (char*)inbuf;
			zfp->strm.bzstrm.avail_in = avail_in;
			zfp->strm.bzstrm.next_out = (char*)outbuf;
			zfp->strm.bzstrm.avail_out = sizeof(outbuf);
			res = BZ2_bzCompress(&(zfp->strm.bzstrm), action);
			if (res != BZ_OK && res != BZ_RUN_OK && res != BZ_FLUSH_OK && res != BZ_FINISH_OK && res != BZ_STREAM_END){
				log_error_ex("bzip2 write error (%d)", res);
				return -1;
			}
			avail_out = zfp->strm.bzstrm.avail_out;
			break;
#endif
#ifndef NO_XZ_SUPPORT
		case COMPRESSOR_XZ:
			zfp->strm.xzstrm.next_in = inbuf;
			zfp->strm.xzstrm.avail_in = avail_in;
			zfp->strm.xzstrm.next_out = outbuf;
			zfp->strm.xzstrm.avail_out = sizeof(outbuf);
			res = lzma_code(&(zfp->strm.xzstrm), action);
			if (res != LZMA_OK && res != LZMA_STREAM_END){
				log_error_ex("xz write error (%d)", res);
				return -1;
			}
			avail_out = zfp->strm.xzstrm.avail_out;
			break;
#endif
		default:
			log_fatal("unsupported");
			return -1;
		}

		write_len = sizeof(outbuf) - avail_out;
		if (fwrite(outbuf, 1, write_len, zfp->fp) != write_len){
			log_efwrite("file");
			return -1;
		}
	}while (avail_in != 0);
	return 0;
}

int zip_compress(const char* infile, const char* outfile, enum COMPRESSOR c_type, int compression_level, unsigned flags){
	struct ZIP_FILE* zfp = NULL;
	FILE* fp_in = NULL;
	int ret = 0;

	if (c_type == COMPRESSOR_LZ4){
		return lz4_compress(infile, outfile, compression_level, flags);
	}

	if (c_type == COMPRESSOR_NONE){
		return copy_file(infile, outfile);
	}

	if (compression_level == 0){
		compression_level = -1;
	}

	fp_in = fopen(infile, "rb");
	if (!fp_in){
		log_efopen(infile);
		ret = -1;
		goto cleanup;
	}

	zfp = zip_open(outfile, 1, c_type, compression_level, flags);
	if (!zfp){
		log_error("Failed to open ZIP_FILE for writing");
		ret = -1;
		goto cleanup;
	}

	if (zip_compress_write(fp_in, zfp) != 0){
		log_error("Failed to write to output file");
		ret = -1;
		goto cleanup;
	}

cleanup:
	zfp ? zip_close(zfp) : 0;
	fp_in ? fclose(fp_in) : 0;
	return ret;
}

static int zip_decompress_read(struct ZIP_FILE* zfp, FILE* fp_out){
	unsigned char inbuf[BUFFER_LEN];
	unsigned char outbuf[BUFFER_LEN];
	int avail_in = 0;
	int avail_out = 0;
	int action;
	size_t write_len = 0;
	int res = 0;
	int res_stream_end = 0;

	switch (zfp->c_type){
#ifndef NO_GZIP_SUPPORT
	case COMPRESSOR_GZIP:
		res_stream_end = Z_STREAM_END;
		zfp->strm.zstrm.next_in = NULL;
		zfp->strm.zstrm.avail_in = 0;
		zfp->strm.zstrm.next_out = outbuf;
		zfp->strm.zstrm.avail_out = sizeof(outbuf);
		action = Z_NO_FLUSH;
		break;
#endif
#ifndef NO_BZIP2_SUPPORT
	case COMPRESSOR_BZIP2:
		res_stream_end = BZ_STREAM_END;
		zfp->strm.bzstrm.next_in = NULL;
		zfp->strm.bzstrm.avail_in = 0;
		zfp->strm.bzstrm.next_out = (char*)outbuf;
		zfp->strm.bzstrm.avail_out = sizeof(outbuf);
		action = BZ_RUN;
		break;
#endif
#ifndef NO_XZ_SUPPORT
	case COMPRESSOR_XZ:
		res_stream_end = LZMA_STREAM_END;
		zfp->strm.xzstrm.next_in = NULL;
		zfp->strm.xzstrm.avail_in = 0;
		zfp->strm.xzstrm.next_out = outbuf;
		zfp->strm.xzstrm.avail_out = sizeof(outbuf);
		action = LZMA_RUN;
		break;
#endif
	default:
		log_error("unsupported");
		return -1;
	}

	do{
		if (avail_in == 0){
			avail_in = read_file(zfp->fp, inbuf, sizeof(inbuf));
			if (avail_in == 0 || feof(zfp->fp)){
				switch (zfp->c_type){
				case COMPRESSOR_GZIP:
					action = Z_FINISH;
					break;
				case COMPRESSOR_BZIP2:
					action = BZ_FINISH;
					break;
				case COMPRESSOR_XZ:
					action = LZMA_FINISH;
					break;
				default:
					;
				}
			}

			switch (zfp->c_type){
#ifndef NO_GZIP_SUPPORT
			case COMPRESSOR_GZIP:
				zfp->strm.zstrm.next_in = inbuf;
				zfp->strm.zstrm.avail_in = avail_in;
				break;
#endif
#ifndef NO_BZIP2_SUPPORT
			case COMPRESSOR_BZIP2:
				zfp->strm.bzstrm.next_in = (char*)inbuf;
				zfp->strm.bzstrm.avail_in = avail_in;
				break;
#endif
#ifndef NO_XZ_SUPPORT
			case COMPRESSOR_XZ:
				zfp->strm.xzstrm.next_in = inbuf;
				zfp->strm.xzstrm.avail_in = avail_in;
				break;
#endif
			default:
				log_fatal("unsupported");
				return -1;
			}
		}

		switch (zfp->c_type){
#ifndef NO_GZIP_SUPPORT
		case COMPRESSOR_GZIP:
			zfp->strm.zstrm.next_out = outbuf;
			zfp->strm.zstrm.avail_out = sizeof(outbuf);
			res = inflate(&(zfp->strm.zstrm), action);
			if (res != Z_OK && res != Z_STREAM_END && res != Z_BUF_ERROR){
				log_error_ex("gzip read error (%d)", res);
				return -1;
			}
			avail_in = zfp->strm.zstrm.avail_in;
			avail_out = zfp->strm.zstrm.avail_out;
			break;
#endif
#ifndef NO_BZIP2_SUPPORT
		case COMPRESSOR_BZIP2:
			zfp->strm.bzstrm.next_out = (char*)outbuf;
			zfp->strm.bzstrm.avail_out = sizeof(outbuf);
			res = BZ2_bzDecompress(&(zfp->strm.bzstrm));
			if (res != BZ_OK && res != BZ_STREAM_END){
				log_error_ex("bzip2 read error (%d)", res);
				return -1;
			}
			avail_in = zfp->strm.xzstrm.avail_in;
			avail_out = zfp->strm.bzstrm.avail_out;
			break;
#endif
#ifndef NO_XZ_SUPPORT
		case COMPRESSOR_XZ:
			zfp->strm.xzstrm.next_out = outbuf;
			zfp->strm.xzstrm.avail_out = sizeof(outbuf);
			res = lzma_code(&(zfp->strm.xzstrm), action);
			if (res != LZMA_OK && res != LZMA_STREAM_END){
				log_error_ex("xz read error (%d)", res);
				return -1;
			}
			avail_in = zfp->strm.xzstrm.avail_in;
			avail_out = zfp->strm.xzstrm.avail_out;
			break;
#endif
		default:
			log_fatal("unsupported");
			return -1;
		}

		write_len = sizeof(outbuf) - avail_out;
		if (fwrite(outbuf, 1, write_len, fp_out) != write_len){
			log_efwrite("file");
			return -1;
		}
	}while (res != res_stream_end);
	return 0;
}

int zip_decompress(const char* infile, const char* outfile, enum COMPRESSOR c_type, unsigned flags){
	struct ZIP_FILE* zfp = NULL;
	FILE* fp_out = NULL;
	int ret = 0;

	if (c_type == COMPRESSOR_LZ4){
		return lz4_decompress(infile, outfile, flags);
	}

	if (c_type == COMPRESSOR_NONE){
		return copy_file(infile, outfile);
	}

	fp_out = fopen(outfile, "wb");
	if (!fp_out){
		log_efopen(outfile);
		ret = -1;
		goto cleanup;
	}

	zfp = zip_open(infile, 0, c_type, 0, flags);
	if (!zfp){
		log_error_ex("Failed to open ZIP_FILE for reading (%s)", infile);
		ret = -1;
		goto cleanup;
	}

	if (zip_decompress_read(zfp, fp_out) != 0){
		log_error("zip_decomp_read failed");
		ret = -1;
		goto cleanup;
	}

cleanup:
	zfp ? zip_close(zfp) : 0;
	fp_out ? fclose(fp_out) : 0;
	return ret;
}

const char* get_compression_extension(enum COMPRESSOR comp){
	switch (comp){
	case COMPRESSOR_GZIP:
		return ".gz";
	case COMPRESSOR_BZIP2:
		return ".bz2";
	case COMPRESSOR_XZ:
		return ".xz";
	case COMPRESSOR_LZ4:
		return ".lz4";
	case COMPRESSOR_NONE:
		return "";
	default:
		log_einval(comp);
		return NULL;
	}
}

enum COMPRESSOR get_compressor_byname(const char* name){
	if (sh_ncasecmp(name, "gzip") == 0 ||
			sh_ncasecmp(name, "gz") == 0){
		return COMPRESSOR_GZIP;
	}
	if (sh_ncasecmp(name, "bzip2") == 0 ||
			sh_ncasecmp(name, "bzip") == 0 ||
			sh_ncasecmp(name, "bz2") == 0 ||
			sh_ncasecmp(name, "bz") == 0){
		return COMPRESSOR_BZIP2;
	}
	if (sh_ncasecmp(name, "xz") == 0 ||
			sh_ncasecmp(name, "lzma2") == 0 ||
			sh_ncasecmp(name, "lzma") == 0){
		return COMPRESSOR_XZ;
	}
	if (sh_ncasecmp(name, "lz4") == 0){
		return COMPRESSOR_LZ4;
	}
	if (sh_ncasecmp(name, "none") == 0 ||
			sh_ncasecmp(name, "off") == 0 ||
			sh_ncasecmp(name, "no") == 0){
		return COMPRESSOR_NONE;
	}
	return COMPRESSOR_INVALID;
}

#undef __ZIP_INTERNAL
