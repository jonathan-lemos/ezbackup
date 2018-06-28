/* stringarray.c -- handles an array of strings
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "stringarray.h"
#include "stringhelper.h"
#include "../log.h"
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

	return 0;
}


int sa_remove(struct string_array* array, size_t index){
	size_t i;

	return_ifnull(array, -1);

	if (index >= array->len){
		log_einval_u(index);
		return -1;
	}

	free(array->strings[index]);

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

static int is_directory(const char* path){
	struct stat st;

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

int sa_contains(const struct string_array* array, const char* str){
	size_t i;

	return_ifnull(array, -1);
	for (i = 0; i < array->len; ++i){
		if (strcmp(array->strings[i], str) == 0){
			return 1;
		}
	}
	return 0;
}

static int cmp(const void* str1, const void* str2){
	return strcmp(*(const char**)str1, *(const char**)str2);
}

void sa_sort(struct string_array* array){
	return_ifnull(array, ;);
	qsort(array->strings, array->len, sizeof(*(array->strings)), cmp);
}

void sa_free(struct string_array* array){
	size_t i;

	if (!array){
		log_debug("array was NULL");
		return;
	}

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

struct string_array* sa_dup(const struct string_array* src){
	struct string_array* ret;
	size_t i;

	ret = sa_new();
	if (!ret){
		log_enomem();
		return NULL;
	}

	for (i = 0; i < src->len; ++i){
		if (sa_add(ret, src->strings[i]) != 0){
			log_enomem();
			sa_free(ret);
			return NULL;
		}
	}

	return ret;
}

int sa_cmp(const struct string_array* sa1, const struct string_array* sa2){
	struct string_array* buf1 = NULL;
	struct string_array* buf2 = NULL;
	size_t i;
	int ret = 0;

	if (sa1->len != sa2->len){
		return (long)sa1->len - (long)sa2->len;
	}

	buf1 = sa_dup(sa1);
	buf2 = sa_dup(sa2);

	if (!buf1 || !buf2){
		log_error("Failed to duplicate string_array");
		ret = 0;
		goto cleanup;
	}

	sa_sort(buf1);
	sa_sort(buf2);

	for (i = 0; i < sa1->len && ret == 0; ++i){
		ret = strcmp(buf1->strings[i], buf2->strings[i]);
	}

cleanup:
	buf1 ? sa_free(buf1) : (void)0;
	buf2 ? sa_free(buf2) : (void)0;
	return ret;
}
