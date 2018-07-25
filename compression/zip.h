/** @file compression/zip.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __COMPRESSION_ZIP_H
#define __COMPRESSION_ZIP_H

/**
 * @brief An enumeration that holds the possible compression algorithms
 */
enum compressor{
	/**
	 * @brief An invalid compressor value.
	 */
	COMPRESSOR_INVALID = 0,
#ifndef NO_GZIP_SUPPORT
	/**
	 * @brief gzip.<br>
	 * This is effectively the standard for Linux/OSX compression.<br>
	 * It is available on almost all Linux/OSX systems by default. Windows users may need to download 7zip to use this algorithm.<br>
	 * <br>
	 * It uses relatively little memory, which makes it great for systems with low amounts of RAM.
	 */
	COMPRESSOR_GZIP,
#endif
#ifndef NO_BZIP2_SUPPORT
	/**
	 * @brief bzip2.<br>
	 * This offers nearly the same compatibillity as gzip while offering higher compression ratios.<br>
	 * As a downside, it uses more memory and takes longer than gzip.<br>
	 * <br>
	 * It is available on almost all Linux/OSX systems by default. Windows users may need to download 7zip to use this algorithm.
	 */
	COMPRESSOR_BZIP2,
#endif
#ifndef NO_XZ_SUPPORT
	/**
	 * @brief xz/lzma2.<br>
	 * This algorithm offers the highest compression ratios while taking the longest time and most memory to compress.<br>
	 * This is not as ubiquitous as gzip/bzip2, but is still available on a majority of Linux systems by default. Linux users may need to install xz to use this algorithm, while OSX/Windows users may need to download 7zip.
	 */
	COMPRESSOR_XZ,
#endif
#ifndef NO_LZ4_SUPPORT
	/**
	 * @brief lz4.<br>
	 * This algorithm offers decent compression ratios while compressing several times as fast as the other algorithms.<br>
	 * This algorithm is only included by default on a handful of Linux distros. Linux users will probably need to install lz4 to use this algorithm, while OSX/Windows users will need to download 7zip-ZS (regular 7zip does not have lz4 support).
	 */
	COMPRESSOR_LZ4,
#endif
	/**
	 * @brief No compressor.<br>
	 * Compression/decompression simply copies the file.
	 */
	COMPRESSOR_NONE
};

/* gzip options */
#define GZIP_NORMAL       (0)      /**< Do not use any special options. This flag is only valid by itself. */
#define GZIP_HUFFMAN_ONLY (1 << 0) /**< Force Huffman enconding only (no string match). This flag is not valid with GZIP_FILTERED or GZIP_RLE */
#define GZIP_FILTERED     (1 << 1) /**< Input data is filtered (many small values that are somewhat random). This flag is not valid with GZIP_HUFFMAN_ONLY or GZIP_RLE. */
#define GZIP_RLE          (1 << 2) /**< Force run-length-encoding. This works best on PNG files. This flag is not valid with GZIP_HUFFMAN_ONLY or GZIP_FILTERED. */
#define GZIP_LOWMEM       (1 << 3) /**< Decrease memory usage. This unfortunately decreases compression ratios as well. */

/* gzip options */
#define BZIP2_NORMAL (0)           /**< Do not use any special options. This flag is only valid by itself. */

/* xz options */
#define XZ_NORMAL  (0)             /**< Do not use any special options. This flag is only valid by itself. */
#define XZ_EXTREME (1 << 0)        /**< Use extreme compression mode. This slightly increases compression ratios, but significantly increases time and memory usage. */

/* lz4 options */
#define LZ4_NORMAL (0)            /**< Do not use any special options. This flag is only valid by itself. */

/**
 * @brief Compresses a file.
 *
 * @param infile Path to the file that should be compressed.
 *
 * @param outfile Path of the resulting output file.<br>
 * If this file already exists, it will be overwritten.
 *
 * @param c_type The compression algorithm to use.
 *
 * @param compression_level A value from 0-9 indicating how much the data should be compressed.<br>
 * A level of 1 runs the fastest but has the lowest compression ratios.<br>
 * A level of 9 runs the slowest but has the highest compression ratios.<br>
 * A level of 0 uses the default value.
 *
 * @param flags Special flags to give to the compression algorithm.<br>
 *
 * @return 0 on success, or negative on failure.<br>
 * On failure, the output file is automatically deleted.
 */
int zip_compress(const char* infile, const char* outfile, enum compressor c_type, int compression_level, unsigned flags);

/**
 * @brief Decompresses a file.
 *
 * @param infile Path to the file that should be decompressed.
 *
 * @param outfile Path of the resulting output file.<br>
 * If this file already exists, it will be overwritten.
 *
 * @param c_type The compression algorithm to use.
 *
 * @param flags Special flags to give to the decompression algorithm.<br>
 * At the moment, decompression does not have any flags available to it.
 *
 * @return 0 on success, or negative on failure.<br>
 * On failure, the output file is automatically deleted.
 */
int zip_decompress(const char* infile, const char* outfile, enum compressor c_type, unsigned flags);

/**
 * @brief Gets a file extension from a compressor value (e.g. COMPRESSOR_GZIP -> ".gz")
 *
 * @param c_type The compressor value.
 *
 * @return The corresponding file extension, or NULL if the compressor value is invalid.
 */
const char* get_compression_extension(enum compressor c_type);

/**
 * @brief Gets a compressor value from its corresponding string representation (e.g. "gzip" -> COMPRESSOR_GZIP)
 *
 * @param name The compressor's string representation.
 *
 * @return The corresponding compressor value, or COMPRESSOR_INVALID if the string did not match any of the available compressors.
 */
enum compressor get_compressor_byname(const char* name);

/**
 * @brief Gets a compressor value's string representation. (e.g. COMPRESSOR_GZIP -> "gzip")
 *
 * @param c_type The compressor value.
 *
 * @return The corresponding string representation, or NULL if the compressor value was invalid.
 */
const char* compressor_tostring(enum compressor c_type);

#endif
