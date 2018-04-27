/* stringarray.c -- handles an array of strings
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "stringarray.h"
#include "stringhelper.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

int sa_add(struct string_array* array, const char* str){
	return_ifnull(array, -1);
	return_ifnull(str, -1);

	array->len++;

	array->strings = realloc(array->strings, array->len * sizeof(*array->strings));
	if (!array->strings){
		log_enomem();
		(array->len)--;
		return -1;
	}

	array->strings[array->len - 1] = sh_dup(str);

	array->strings[array->len - 1] = malloc(strlen(str) + 1);
	if (!array->strings[array->len - 1]){
		log_enomem();
		return -1;
	}

	strcpy(array->strings[array->len - 1], str);
	return 0;
}


int sa_remove(struct string_array* array, size_t index){
	size_t i;

	return_ifnull(array, -1);

	if (index >= array->len){
		log_einval_u(index);
		return -1;
	}

	for (i = index; i < array->len - 1; ++i){
		array->strings[i] = array->strings[i + 1];
	}
	array->len--;

	array->strings = realloc(array->strings, array->len * sizeof(*array->strings));
	if (!array->strings && array->len != 0){
		log_enomem();
		return -1;
	}
	return 0;
}

int is_directory(const char* path){
	struct stat st;

	return_ifnull(path, -1);

	if (stat(path, &st) != 0){
		log_estat(path);
		return 0;
	}

	return S_ISDIR(st.st_mode);
}

size_t sa_sanitize_directories(struct string_array* array){
	size_t n_removed = 0;
	size_t i;

	return_ifnull(array, 0);

	for (i = 0; i < array->len; ++i){
		if (!is_directory(array->strings[i])){
			sa_remove(array, i);
			i--;
			n_removed++;
		}
	}
	return n_removed;
}

int cmp(const void* str1, const void* str2){
	return strcmp(*(const char**)str1, *(const char**)str2);
}

void sa_sort(struct string_array* array){
	return_ifnull(array, ;);
	qsort(array->strings, array->len, sizeof(*(array->strings)), cmp);
}

void sa_free(struct string_array* array){
	size_t i;

	return_ifnull(array, ;);

	for (i = 0; i < array->len; ++i){
		free(array->strings[i]);
	}
	free(array->strings);
	free(array);
}

struct string_array* sa_new(void){
	struct string_array* ret;
	ret = malloc(sizeof(*ret));
	if (!ret){
		log_enomem();
		return NULL;
	}
	ret->strings = NULL;
	ret->len = 0;
	return ret;
}
