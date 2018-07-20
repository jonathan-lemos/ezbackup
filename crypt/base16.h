/** @file crypt/base16.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CRYPT_BASE16_H
#define __CRYPT_BASE16_H

/**
 * @brief Converts a series of bytes to a null-terminated base16 string.
 *
 * @param bytes The raw data to convert.
 *
 * @param len The length of the raw data in bytes.
 *
 * @param out A pointer to a string that will contain the base16 representation of the bytes.
 * This string is null-terminated, so strlen() will get the length of the string.
 * This string will be set to NULL if this function fails.
 * This string must be free()'d when no longer in use.
 *
 * @return 0 on success, or negative on failure.
 */
int to_base16(const void* bytes, unsigned len, char** out);

/**
 * @brief Converts a null-terminated base16 string to a series of raw bytes.
 *
 * @param hex The base16 string to convert.
 * This base16 string must be null-terminated.
 * Characters 0-9, A-F, and a-f are valid.
 *
 * @param out A pointer to a location that will be filled with the raw byte representation of the base16 string.
 * This will be set to NULL if this function fails.
 * This value must be free()'d when no longer in use.
 *
 * @param out_len A pointer to an integer that will contain the length of the output in bytes.
 * This will be set to 0 if this function fails.
 * This value can be NULL, in which case it is not used.
 *
 * @return 0 on success, or negative on failure.
 */
int from_base16(const char* hex, void** out, unsigned* out_len);

#endif
