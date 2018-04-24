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

int sa_add(struct string_array* array, const char* str);
int sa_remove(struct string_array* array, int index);
int is_directory(const char* path);
size_t sa_sanitize_directories(struct string_array* array);
void sa_sort(struct string_array* array);
struct string_array* sa_new(void);
void sa_free(struct string_array* array);

#endif
