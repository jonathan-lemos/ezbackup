/* stringhelper.c -- helper functions for c-strings
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "../log.h"
#include <stdlib.h>
#include <string.h>

char* sh_new(void){
	return calloc(1, 1);
}

char* sh_dup(const char* in){
	char* ret;

	return_ifnull(in, NULL);

	ret = malloc(strlen(in) + 1);
	if (!ret){
		log_enomem();
		return NULL;
	}
	strcpy(ret, in);
	return ret;
}

char* sh_concat(char* in, const char* extension){
	return_ifnull(in, NULL);
	return_ifnull(extension, NULL);

	in = realloc(in, strlen(in) + strlen(extension) + 1);
	if (!in){
		log_enomem();
		return NULL;
	}
	strcat(in, extension);
	return in;
}

const char* sh_file_ext(const char* in){
	size_t ptr = 0;

	return_ifnull(in, NULL);

	while ((ptr = strcspn(in, ".")) != strlen(in)){
		in += ptr + 1;
		ptr = 0;
	}
	return in;
}

const char* sh_file_name(const char* in){
	size_t ptr = 0;

	return_ifnull(in, NULL);

	while ((ptr = strcspn(in, "/")) != strlen(in)){
		in += ptr + 1;
		ptr = 0;
	}
	return in;
}

int sh_starts_with(const char* haystack, const char* needle){
	size_t i;

	if (strlen(haystack) < strlen(needle)){
		return 0;
	}

	for (i = 0; i < strlen(needle); ++i){
		if (haystack[i] != needle[i]){
			return 0;
		}
	}
	return 1;
}
