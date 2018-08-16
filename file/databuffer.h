/** @file file/databuffer.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __DATABUFFER_H
#define __DATABUFFER_H

#include "../attribute.h"
#include <stddef.h>

#ifndef DB_DEFAULT_CAPACITY
#define DB_DEFAULT_CAPACITY (256) /**< Default internal capacity of a data buffer */
#endif

/**
 * @brief An automatically resizing data buffer.<br>
 * Resizes are conducted efficiently while not wasting much space.
 */
struct databuffer{
	unsigned char* data; /**< The data to be stored */
	size_t size;         /**< The current size of the data. This also is the index of the next character. */
	size_t capacity;     /**< The current capacity of the data buffer. */
};

/**
 * @brief Creates a new data buffer structure on the heap.<br>
 * This is slower than db_fill(), but exists beyond the lifetime of the stack.
 * @see db_fill()
 *
 * @return A new data buffer structure.<br>
 * This must be freed with db_free() when no longer in use.
 * @see db_free()
 */
struct databuffer* db_create(void) EZB_MALLOC_LIKE;

/**
 * @brief Fills a data buffer structure on the stack with its default contents.<br>
 * If you want to create a data buffer structure on the heap, use db_create()
 * @see db_create()
 *
 * @param db A pointer to a data buffer structure on the stack.
 * The contents of this structure must be freed with db_free_contents(), not db_free().
 * @see db_free_contents()
 *
 * @return 0 on success, negative on failure.
 */
int db_fill(struct databuffer* db);

/**
 * @brief Resizes a data buffer structure.<br>
 * Only use this if you are trying to reduce the size of the structure.<br>
 * The db_concat()/db_concat_char() functions automatically resize the structure.
 *
 * @param db The data buffer structure to resize.
 *
 * @param needed_capacity The capacity needed.<br>
 * The capacity delivered may be higher than the needed capacity.
 *
 * @return 0 on success, negative on failure.<br>
 * On failure, the existing data buffer structure is not modified and still must be freed with db_free()/db_free_contents()
 */
int db_resize(struct databuffer* db, size_t needed_capacity);

/**
 * @brief Concatenates data to a data buffer.<br>
 *
 * @param db The data buffer to add to.
 *
 * @param data The data to add.
 *
 * @param len The length of the data to add in bytes.
 *
 * @return 0 on success, negative on failure.
 */
int db_concat(struct databuffer* db, unsigned char* data, size_t len);

/**
 * @brief Concatenates a single character to a data buffer.<br>
 *
 * @param db The data buffer to add to.
 *
 * @param c The character to add.
 *
 * @return 0 on success, negative on failure.
 */
int db_concat_char(struct databuffer* db, unsigned char c);

/**
 * @brief Frees the contents of a data buffer.<br>
 * Only use this with a data buffer allocated through db_fill().
 * Otherwise, use db_free().
 * @see db_fill()
 *
 * @param db The data buffer to have its contents free.
 */
void db_free_contents(struct databuffer* db);

/**
 * @brief Frees a data buffer.<br>
 * Only use this with a data buffer allocated through db_create().
 * Otherwise, use db_free_contents().
 * @see db_create()
 *
 * @param db The data buffer to free.
 */
void db_free(struct databuffer* db);

#endif
