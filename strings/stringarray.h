/* stringarray.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __STRINGARRAY_H
#define __STRINGARRAY_H

#include <stddef.h>

struct string_array{
	char** strings;
	size_t len;
};

struct string_array* sa_new(void);
int sa_add(struct string_array* array, const char* str);
int sa_insert(struct string_array* array, const char* str, size_t index);
int sa_remove(struct string_array* array, size_t index);
int sa_contains(const struct string_array* array, const char* str);
void sa_sort(struct string_array* array);
void sa_free(struct string_array* array);
int sa_cmp(const struct string_array* sa1, const struct string_array* sa2);
void sa_to_raw_array(struct string_array* arr, char*** out, size_t* out_len);
int sa_merge(struct string_array* dst, struct string_array* src);

size_t sa_sanitize_directories(struct string_array* array);
struct string_array* sa_get_parent_dirs(const char* directory);

#endif
