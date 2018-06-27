/* stringarray.h -- handles an array of strings
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
int sa_remove(struct string_array* array, size_t index);
int sa_contains(const struct string_array* array, const char* str);
size_t sa_sanitize_directories(struct string_array* array);
void sa_sort(struct string_array* array);
void sa_free(struct string_array* array);
int sa_cmp(const struct string_array* sa1, const struct string_array* sa2);

#endif
