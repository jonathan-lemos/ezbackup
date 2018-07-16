/* zip.h
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
enum COMPRESSOR{
	/**
	 * @brief An invalid compressor value.
	 */
	COMPRESSOR_INVALID = 0,
#ifndef NO_GZIP_SUPPORT
	/**
	 * @brief gzip.
	 * This is effectively the standard for Linux/OSX compression.
	 * It is available on almost all Linux/OSX systems by default. Windows users will need to download 7zip to use this algorithm.
	 *
	 * It uses relatively little memory, which makes it great for systems with low amounts of RAM.
	 */
	COMPRESSOR_GZIP,
#endif
#ifndef NO_BZIP2_SUPPORT
	/**
	 * @brief bzip2.
	 * This offers nearly the same compatibillity as gzip while offering higher compression ratios.
	 * As a downside, it uses more memory and takes longer than gzip.
	 */
	COMPRESSOR_BZIP2,
#endif
#ifndef NO_XZ_SUPPORT
	/**
	 * @brief xz/lzma2.
	 * This algorithm offers the highest compression ratios while taking the longest time and most memory to compress.
	 * This is not as ubiquitous as gzip/bzip2, but is still available on a majority of Linux systems by default. Linux users may need to install xz to use this algorithm, while OSX/Windows users will need to download 7zip.
	 */
	COMPRESSOR_XZ,
#endif
#ifndef NO_LZ4_SUPPORT
	/**
	 * @brief lz4.
	 * This algorithm offers decent compression ratios while compressing several times as fast as the other algorithms.
	 * This algorithm is only included by default on a handful of Linux distros. Linux users will probably need to install lz4 to use this algorithm, while OSX/Windows users will need to download 7zip-ZS (regular 7zip does not have lz4 support).
	 */
	COMPRESSOR_LZ4,
#endif
	/**
	 * @brief No compressor.
	 * Compression/decompression simply copies the file.
	 */
	COMPRESSOR_NONE
};

#define GZIP_NORMAL (0x0)       /**< gzip: Do not use any special options. This flag is only valid by itself. */
#define GZIP_HUFFMAN_ONLY (0x1) /**< gzip: Force Huffman encoding only (no string match). */
#define GZIP_FILTERED (0x2)     /**< gzip: Input data is filtered (many small values that are somewhat random) */
#define GZIP_RLE (0x4)          /**< gzip: Force run-length-encoding. This works best on PNG files. */
#define GZIP_LOWMEM (0x8)       /**< gzip: Use a lower amount of memory. This decreases compression ratio. */

#define BZIP2_NORMAL (0x0)      /**< bzip2: Do not use any special options. This flag is only valid by itself. */

#define XZ_NORMAL (0x0)         /**< xz: Do not use any special options. This flag is only valid by itself. */
#define XZ_EXTREME (0x1)        /**< xz: Use extreme compression mode. This marginally increases compression ratios, but greatly increases time and memory usage. */

#define LZ4_NORMAL (0x0)        /**< lz4: Do not use any special options. This flag is only valid by itself. */

/**
 * @brief Compresses a file.
 *
 * @param infile Path to the file that should be compressed.
 *
 * @param outfile Path of the resulting output file.
 * If this file already exists, it will be overwritten.
 *
 * @param c_type The compression algorithm to use.
 *
 * @param compression_level A value from 0-9 indicating how much the data should be compressed.
 * A level of 1 runs the fastest but has the lowest compression ratios.
 * A level of 9 runs the slowest but has the highest compression ratios.
 * A level of 0 gives the default value.
 *
 * @param flags Special flags to give to the compression algorithm.
 * Flags can be combined using the "|" operator (e.g. GZIP_RLE | GZIP_LOWMEM)
 *
 * @return 0 on success, or negative on failure.
 * On failure, the output file is automatically deleted.
 */
int zip_compress(const char* infile, const char* outfile, enum COMPRESSOR c_type, int compression_level, unsigned flags);

/**
 * @brief Decompresses a file.
 *
 * @param infile Path to the file that should be decompressed.
 *
 * @param outfile Path of the resulting output file.
 * If this file already exists, it will be overwritten.
 *
 * @param c_type The compression algorithm to use.
 *
 * @param flags Special flags to give to the decompression algorithm.
 * At the moment, decompression does not have any flags available to it.
 *
 * @return 0 on success, or negative on failure.
 * On failure, the output file is automatically deleted.
 */
int zip_decompress(const char* infile, const char* outfile, enum COMPRESSOR c_type, unsigned flags);

/**
 * @brief Gets a file extension from a compressor value (e.g. COMPRESSOR_GZIP -> ".gz")
 *
 * @param c_type The compressor value.
 *
 * @return The corresponding file extension, or NULL if the compressor value is invalid.
 */
const char* get_compression_extension(enum COMPRESSOR c_type);

/**
 * @brief Gets a compressor value from its corresponding string representation (e.g. "gzip" -> COMPRESSOR_GZIP)
 *
 * @param name The compressor's string representation.
 *
 * @return The corresponding compressor value, or COMPRESSOR_INVALID if the string did not match any of the available compressors.
 */
enum COMPRESSOR get_compressor_byname(const char* name);

/**
 * @brief Gets a compressor value's string representation. (e.g. COMPRESSOR_GZIP -> "gzip")
 *
 * @param c_type The compressor value.
 *
 * @return The corresponding string representation, or NULL if the compressor value was invalid.
 */
const char* compressor_tostring(enum COMPRESSOR c_type);

#endif
