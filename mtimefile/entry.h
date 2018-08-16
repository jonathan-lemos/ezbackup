/** @file mtimefile/entry.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This softare may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#define ENTRY_ARR_DEFAULT_CAPACITY (256)

#include "../attribute.h"
#include <time.h>

/**
 * @brief An mtime file entry.
 */
struct entry{
	char* file;   /**< The filename */
	time_t mtime; /**< The modification time of the file */
};

struct entry_array{
	struct entry** entries; /**< A list of entries */
	size_t size;            /**< The amount of entries in the array */
	size_t capacity;        /**< The maximum capacity of the array. Do not modify this parameter manually. */
};

/**
 * @brief Creates a new entry array.
 *
 * @return A new entry array.<br>
 * This must be freed with entry_arr_free()
 * @see entry_arr_free()
 */
struct entry_array* entry_arr_new(void) EZB_MALLOC_LIKE;

/**
 * @brief Adds an entry to an entry array.
 *
 * @param ea The entry array to add to.<br>
 * Use entry_arr_new() to generate a new array.
 * @see entry_arr_new()
 *
 * @param e The entry to add.
 *
 * @return 0 on success, negative on failure.<br>
 * On failure, the existing array is not modified and still must be freed with entry_arr_free()
 */
int entry_arr_add(struct entry_array* ea, struct entry* e);

/**
 * @brief Frees an entry array allocated with entry_arr_new()
 *
 * @param ea The entry array to free.
 * This parameter can be NULL, in which case this function does nothing.
 */
void entry_arr_free(struct entry_array* ea);
