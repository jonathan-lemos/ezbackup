/** @file strings/stringarray.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __STRINGARRAY_H
#define __STRINGARRAY_H

#include <stddef.h>

/**
 * @brief An array of strings.
 * The length is automatically managed.
 */
struct string_array{
	char** strings; /**< The strings in the array. */
	size_t len;     /**< The length of the array. */
};

/**
 * @brief Creates a blank string array.
 *
 * @return A new string array.<br>
 * This array must be freed with sa_free(), sa_to_raw_array(), or sa_merge() when no longer in use.
 * @see sa_free()
 * @see sa_to_raw_array()
 * @see sa_merge()
 */
struct string_array* sa_new(void);

/**
 * @brief Adds a string to a string array.
 *
 * @param array The array to add a string to.
 *
 * @param str The string to add.
 *
 * @return 0 on success, or negative on failure.<br>
 * On failure, the array's contents are unchanged.
 */
int sa_add(struct string_array* array, const char* str);

/**
 * @brief Insets a string into a string array at a specific index.
 *
 * @param array The array to add a string to.
 *
 * @param str The string to add.
 *
 * @param index The index to insert the string into.
 *
 * @return 0 on success, or negative on failure.<br>
 * On failure, the array's contents are unchanged.
 */
int sa_insert(struct string_array* array, const char* str, size_t index);

/**
 * @brief Removes a string from an array.
 *
 * @param array The array to remove a string from.
 *
 * @param index The index of the string to remove.
 *
 * @return 0 on success or negative on failure.<br>
 * On failure, the array's contents are unchanged.
 */
int sa_remove(struct string_array* array, size_t index);

/**
 * @brief Returns true if the array contains the string and false if it doesn't.
 *
 * @param array The array to find the string within.
 *
 * @param str The string to find.
 *
 * @return Positive if the array contains the string, 0 if it doesn't.
 */
int sa_contains(const struct string_array* array, const char* str);

/**
 * @brief Sorts a string array in strcmp() order.
 *
 * @param array The array to sort.
 *
 * @return void
 */
void sa_sort(struct string_array* array);

/**
 * @brief Resets a string array so that it is identical to one returned by sa_new().<br>
 * This frees all memory associated with the string array's contents.
 *
 * @param array The array to reset.<br>
 * This can be NULL, in which case the function does nothing.
 *
 * @return void
 */
void sa_reset(struct string_array* array);

/**
 * @brief Frees all memory associated with a string array.
 *
 * @param array The array to free.<br>
 * This can be NULL, in which case the function does nothing.
 *
 * @return void
 */
void sa_free(struct string_array* array);

/**
 * @brief Compares the contents of two string arrays.<br>
 * This function does not care about the order of the strings; it only cares about the contents of the strings.
 *
 * @param sa1 The first string array.
 *
 * @param sa2 The second string array.
 *
 * @return 0 if each array contains the same strings, non-zero if they don't.
 */
int sa_cmp(const struct string_array* sa1, const struct string_array* sa2);

/**
 * @brief Converts a string array to a raw array and length.
 *
 * @param arr The string array to convert.<br>
 * This array will be freed after this function completes and should not be reused.
 *
 * @param out A pointer to a raw string array that will be filled by this function.
 *
 * @param out_len A pointer to an integer that will contain the length of the output array.
 *
 * @return void
 */
void sa_to_raw_array(struct string_array* arr, char*** out, size_t* out_len);

/**
 * @brief Merges two string arrays into one.
 * The contents of the source string array will be appended to the destination string array.
 *
 * @param dst The array to be merged into.
 *
 * @param src The array that will be merged into the destination array.<br>
 * If the function succeeds, this array will be freed and should not be reused.<br>
 * Otherwise, this array is still valid.
 *
 * @return 0 on success, or negative on failure.<br>
 * On failure, neither array is changed.
 */
int sa_merge(struct string_array* dst, struct string_array* src);

/**
 * @brief Removes all entries from a string array that do not correspond to a valid directory.
 *
 * @param array The array to sanitize.
 *
 * @return The number of directories removed.
 */
size_t sa_sanitize_directories(struct string_array* array);

/**
 * @brief Splits a directory into its parent directories (e.g. "/dir1/dir2/dir3" -> {"/dir1", "/dir1/dir2", "/dir1/dir2/dir3"})
 *
 * @param directory The directory to split.<br>
 * This directory does not necessarily have to exist as long as it could be a valid path.<br>
 * The directory may or may not start with a '/' character.
 *
 * @return A string array corresponding to the parent directories, or NULL on failure.
 */
struct string_array* sa_get_parent_dirs(const char* directory);

#endif
