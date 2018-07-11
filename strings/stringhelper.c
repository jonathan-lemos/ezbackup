/* stringhelper.c -- helper functions for c-strings
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "../log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

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

char* sh_concat_path(char* in, const char* extension){
	return_ifnull(in, NULL);
	return_ifnull(extension, NULL);

	if (in[strlen(in) - 1] != '/' && (in = sh_concat(in, "/")) == NULL){
		log_error("Failed to concatenate trailing slash to in");
		return NULL;
	}

	if (extension[0] == '/'){
		extension++;
	}

	if ((in = sh_concat(in, extension)) == NULL){
		log_error("Failed to concatenate extension to in");
		return NULL;
	}

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

const char* sh_filename(const char* in){
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

char* sh_getcwd(void){
	char* cwd = NULL;
	int cwd_len = 256;

	cwd = malloc(cwd_len);
	if (!cwd){
		log_enomem();
		return NULL;
	}
	while (getcwd(cwd, cwd_len) == NULL){
		if (errno != ERANGE){
			log_error_ex("Failed to get current directory (%s)", strerror(errno));
			free(cwd);
			return NULL;
		}
		cwd_len *= 2;
		cwd = realloc(cwd, cwd_len);
		if (!cwd){
			log_enomem();
			return NULL;
		}
	}
	cwd = realloc(cwd, strlen(cwd) + 1);
	if (!cwd){
		log_enomem();
		return NULL;
	}

	return cwd;
}

int sh_cmp_nullsafe(const char* str1, const char* str2){
	if (!str1 && !str2){
		return 0;
	}
	if ((!str1) != (!str2)){
		return str1 == NULL ? 1 : -1;
	}
	return strcmp(str1, str2);
}

int sh_ncasecmp(const char* str1, const char* str2){
	char* s1 = NULL;
	char* s2 = NULL;
	int res;
	size_t i;

	if (!str1 || !str2){
		return sh_cmp_nullsafe(str1, str2);
	}

	s1 = sh_dup(str1);
	s2 = sh_dup(str2);
	if (!s1 || !s2){
		free(s1);
		free(s2);
		log_error("Failed to duplicate one or more strings");
		return 0;
	}

	for (i = 0; i < strlen(s1); ++i){
		if (s1[i] >= 'A' && s1[i] <= 'Z'){
			s1[i] += 'a' - 'A';
		}
	}

	for (i = 0; i < strlen(s2); ++i){
		if (s2[i] >= 'A' && s2[i] <= 'Z'){
			s2[i] += 'a' - 'A';
		}
	}

	res = strcmp(s1, s2);

	free(s1);
	free(s2);
	return res;
}

char* sh_sprintf(const char* format, ...){
	char* ret = NULL;
	int ret_len;
	va_list ap;
	va_start(ap, format);

	ret_len = vsnprintf(ret, 0, format, ap);
	if (ret_len < 0){
		log_error("vsnprintf() encoding error");
		va_end(ap);
		return NULL;
	}

	ret = malloc(ret_len + 1);
	if (!ret){
		log_enomem();
		va_end(ap);
		return NULL;
	}

	if (vsnprintf(ret, ret_len + 1, format, ap) != ret_len){
		log_error("vsnprintf() write error");
		free(ret);
		va_end(ap);
		return NULL;
	}

	va_end(ap);
	return ret;
}
