/* options_file.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __OPTIONS_FILE_H
#define __OPTIONS_FILE_H

#include <stdio.h>
#include <stddef.h>

/**
 * @brief An option entry structure.
 */
struct opt_entry{
	char* key;        /**< A dynamically-allocated string corresponding to the key. */
	void* value;      /**< A dynamically-allocated series of bytes corresponding to the value. */
	size_t value_len; /**< The length of the byte array. */
};

/**
 * @brief Creates a new options file at the specified path.
 *
 * @param path The path of the options file to create.
 *
 * @return A file pointer corresponding to the created file, or NULL on failure.
 * This file must only be written to with add_option_to_file().
 * This file pointer must be freed with fclose() when no longer in use.
 * Failure to do so will cause corruption in the file along with a memory leak.
 * @see add_option_tofile()
 */
FILE* create_option_file(const char* path);

/**
 * @brief Adds an option to a file.
 * Said option can be accessed through its key.
 *
 * @param fp A file pointer returned by create_option_file()
 * @see create_option_file()
 *
 * @param key The key used to access the value.
 * This cannot be NULL.
 *
 * @param value The value to be written.
 * This can be NULL.
 *
 * @param value_len The length of the value to write.
 * If the value is NULL, this must be 0.
 *
 * @return 0 on success, or negative on failure.
 */
int add_option_tofile(FILE* fp, const char* key, const void* value, size_t value_len);

/**
 * @brief Reads the keys/values stored in an option file.
 * This option file must have had its entries created through add_option_tofile()
 * The output will be sorted in strcmp() order by their keys.
 * @see add_option_tofile()
 *
 * @param option_file Path to a previously created option file.
 *
 * @param out A pointer to an array of entries that will be filled.
 * This cannot be NULL, but can point to NULL.
 * If this function fails, the array will be set to NULL.
 * This array must be freed with free_opt_entry_array() when no longer in use.
 * @see free_opt_entry_array()
 *
 * @param len_out A pointer to an integer that will contain the length of the output array.
 * This cannot be NULL.
 * This will be set to 0 if the function fails.
 *
 * @return 0 on success, or negative on failure.
 */
int read_option_file(const char* option_file, struct opt_entry*** out, size_t* len_out);

/**
 * @brief Searches a list of option entries for the corresponding key, returning its index if said key exists.
 * This list must be returned by read_option_file() or otherwise sorted.
 * @see read_option_file()
 *
 * @param entries The entries to search within.
 *
 * @param len The length of the entries.
 *
 * @param key The key to search for.
 *
 * @return The index of the entry corresponding to the key, or negative if the entry could not be found or there was an error.
 */
int binsearch_opt_entries(const struct opt_entry* const* entries, size_t len, const char* key);

/**
 * @brief Frees all memory associated with an array of option entries.
 *
 * @param entries The entries to free.
 * This can be NULL, in which case this function does nothing.
 *
 * @param len The length of the option entry array.
 *
 * @return void
 */
void free_opt_entry_array(struct opt_entry** entries, size_t len);

#endif
