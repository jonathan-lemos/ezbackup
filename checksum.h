/** @file checksum.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CHECKSUM_H
#define __CHECKSUM_H

#include <stdio.h>
#include <openssl/evp.h>

/**
 * @brief Returns an EVP_MD* object for a given string.
 * @see checksum()
 * @see add_checksum_to_file()
 *
 * @param hash_name Name of the digest algorithm (e.g. "SHA1").
 *
 * @return The corresponding EVP_MD*.
 */
const EVP_MD* get_evp_md(const char* hash_name);

/**
 * @brief Calculates the checksum for a given file.
 *
 * @param file Path of the file to calculate the checksum for.
 *
 * @param algorithm The digest algorithm to use.
 * @see get_evp_md()
 *
 * @param out A pointer to the location of the output checksum.<br>
 * Said location will be set to NULL if this function fails.<br>
 * The output checksum will be a series of raw bytes and not a hexadecimal string.<br>
 * This value must be free()'d when no longer in use.
 * @see bytes_to_hex()
 *
 * @param len The length of the output checksum. Will be 0 if this function fails.
 *
 * @return 0 on success, negative on failure
 */
int checksum(const char* file, const EVP_MD* algorithm, unsigned char** out, unsigned* len);

/**
 * @brief Converts a series of raw bytes into a null-terminated hexadecimal string.
 * @see checksum()
 *
 * @param bytes The raw bytes to convert into a hexadecimal string.
 *
 * @param len The length of the raw bytes to convert.
 *
 * @param out A pointer to the null-terminated output string.<br>
 * Said location will be set to NULL if this function fails.<br>
 * This value must be free()'d when no longer in use.
 *
 * @return 0 on success, negative on failure
 */
int bytes_to_hex(const unsigned char* bytes, unsigned len, char** out);

/**
 * @brief Adds a file's checksum to a checksum list.
 *
 * Said checksum will have the following format:<br>
 * /path/to/file\0ABCDEF123456\n
 *
 * @param file The file to calculate a checksum for.
 *
 * @param algorithm The digest algorithm to use.
 * @see get_evp_md()
 *
 * @param out The checksum list to add the file's checksum to.<br>
 * This FILE* must be opened in writing binary ("wb") mode.<br>
 *
 * @param prev_checksums An optional previous sorted checksum list to check if the file's contents were changed or not.<br>
 * Set this parameter to NULL if there is no previous checksum list.<br>
 * Otherwise, this FILE* must be opened in reading binary ("rb") mode.<br>
 * Undefined behavior if this list is not sorted.
 * @see sort_checksum_file()
 *
 * @return 0 on success, positive if the file was unchanged from prev_checksums, negative on failure.
 */
int add_checksum_to_file(const char* file, const EVP_MD* algorithm, FILE* out, FILE* prev_checksums);

/**
 * @brief Sorts a checksum list in strcmp() order by filename.
 *
 * @param in_out The checksum list to sort.
 * @see add_checksum_to_file()
 *
 * @return 0 on success, negative on failure.<br>
 * On failure, in_out will be unchanged.
 */
int sort_checksum_file(const char* in_out);

/**
 * @brief Searches a sorted checksum list for a filename, and returns its checksum if it exists.
 *
 * This function just calls search_file()
 * @see search_file()
 *
 * @param fp A sorted checksum list.<br>
 * This FILE* must be opened in reading binary ("rb") mode.<br>
 * Undefined behavior if this list is not sorted.<br>
 * @see sort_checksum_file()
 *
 * @param key The filename to search for.
 *
 * @param checksum A pointer to the output checksum location.<br>
 * The output will be a null-terminated hexadecimal checksum string, or NULL if the key could not be found or there was an error.
 *
 * @return 0 on success, positive if the checksum could not be found, negative on error.
 */
int search_for_checksum(FILE* fp, const char* key, char** checksum);

/**
 * @brief Creates a list of removed files since the creation of a previous checksum file.
 *
 * @param checksum_file The filename of a previously created checksum file.
 *
 * @param out_file The filename of the resulting output file.<br>
 * This file will be overwritten if it exists.<br>
 * Said file will contain a list of all files with entries in the checksum_file that no longer exist.<br>
 * The entries will be in the following format:
 *
 * /path/to/file1\n<br>
 * /path/to/file2\n
 *
 * @return 0 on success, negative on failure
 */
int create_removed_list(const char* checksum_file, const char* out_file);

/* TODO: return an integer since there's multiple reasons for NULL */
/**
 * @brief Gets the next entry in a removed file list
 * @see create_removed_list
 *
 * @param fp A removed file list<br>
 * This FILE* must be opened in reading binary ("rb") mode
 *
 * @return A null-terminated string containing the next filename in the removed file list, or NULL on end-of-file or error.<br>
 * This value must be free()'d when no longer in use.
 */
char* get_next_removed(FILE* fp);

#endif
