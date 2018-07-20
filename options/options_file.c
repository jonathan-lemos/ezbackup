/** @file options/options_file.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "options_file.h"
#include "../log.h"
#include "../strings/stringhelper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* file format:
 * [Options]\n
 * KEY=XXXXXXXXVALUE\n
 * KEY2=XXXXXXXXVALUE2\n
 *
 * XXXXXXXX is length of VALUE as a size_t
 */

static void free_opt_entry(struct opt_entry* entry){
	if (!entry){
		return;
	}
	free(entry->key);
	free(entry->value);
	free(entry);
}

static int add_opt_entry(struct opt_entry*** entries, size_t* len, struct opt_entry* val){
	(*len)++;
	*entries = realloc(*entries, (*len) * sizeof(*entries));
	if (!(*entries)){
		log_enomem();
		(*len) = 0;
		return -1;
	}
	(*entries)[(*len) - 1] = val;
	return 0;
}

/* this must be struct opt_entry*** because realloc may move the pointer
 *
 * remember this the next time you spend two days tracking down a memory error */
static int remove_opt_entry(struct opt_entry*** entries, size_t* len, size_t index){
	size_t i;

	free_opt_entry((*entries)[index]);
	for (i = index; i < (*len) - 1; ++i){
		entries[i] = entries[i + 1];
	}

	(*len)--;
	(*entries) = realloc((*entries), (*len) * sizeof(*entries));
	if (!(*entries)){
		log_enomem();
		(*len) = 0;
		return -1;
	}

	return 0;
}

static int remove_null_entries(struct opt_entry*** entries, size_t* len){
	size_t i;
	for (i = 0; i < *len; ++i){
		if (!((*entries)[i]) && remove_opt_entry(entries, len, i)){
			log_warning("Failed to remove NULL entry from list");
			return -1;
		}
	}
	return 0;
}

/*
static int pop_opt_entry(struct opt_entry** entries, size_t* len){
	return remove_opt_entry(entries, len, *len - 1);
}
*/

static int cmp(const void* e1, const void* e2){
	return strcmp((*(struct opt_entry**)e1)->key, (*(struct opt_entry**)e2)->key);
}

static void sort_opt_entries(struct opt_entry** entries, size_t len){
	qsort(entries, len, sizeof(*entries), cmp);
}

static struct opt_entry* read_entry(FILE* fp){
	long fpos_begin;
	long fpos_end;
	int c;
	struct opt_entry* ret;

	ret = calloc(1, sizeof(*ret));
	if (!ret){
		log_enomem();
		return NULL;
	}

	/* read bytes until the '=' */
	fpos_begin = ftell(fp);
	while ((c = fgetc(fp)) != '=' && c != EOF);
	if (c == EOF){
		log_info("read_entry reached EOF");
		free_opt_entry(ret);
		return NULL;
	}
	/* - 1 to ignore the '=' */
	fpos_end = ftell(fp) - 1;

	/* + 1 for '\0' */
	ret->key = malloc(fpos_end - fpos_begin + 1);
	if (!ret->key){
		log_enomem();
		free_opt_entry(ret);
		return NULL;
	}

	/* read the key */
	fseek(fp, fpos_begin, SEEK_SET);
	fread(ret->key, 1, fpos_end - fpos_begin, fp);
	ret->key[fpos_end - fpos_begin] = '\0';

	/* advance fp beyond '=' */
	fgetc(fp);

	/* read the length of value (raw bytes) */
	fread(&(ret->value_len), sizeof(ret->value_len), 1, fp);

	/* now read the value */
	if (ret->value_len > 0){
		ret->value = malloc(ret->value_len);
		if (!ret->value){
			log_enomem();
			free_opt_entry(ret);
			return NULL;
		}
		fread(ret->value, 1, ret->value_len, fp);
	}
	else{
		ret->value = NULL;
	}

	if (ferror(fp) != 0){
		log_efread("file");
		free_opt_entry(ret);
		return NULL;
	}

	/* advance fp beyond '\n' */
	fgetc(fp);

	return ret;
}

static int write_entry(FILE* fp, struct opt_entry* entry){
	fwrite(entry->key, 1, strlen(entry->key), fp);
	fwrite("=", 1, 1, fp);
	fwrite(&entry->value_len, sizeof(entry->value_len), 1, fp);
	if (entry->value){
		fwrite(entry->value, 1, entry->value_len, fp);
	}
	fwrite("\n", 1, 1, fp);
	return ferror(fp) != 0;
}

FILE* create_option_file(const char* path){
	FILE* fp;

	fp = fopen(path, "w");
	if (!fp){
		log_efopen(path);
		return NULL;
	}

	fprintf(fp, "[Options]\n");
	if (ferror(fp) != 0){
		log_efwrite(path);
		fclose(fp);
		return NULL;
	}

	return fp;
}

int add_option_tofile(FILE* fp, const char* key, const void* value, size_t value_len){
	struct opt_entry* entry;

	entry = malloc(sizeof(*entry));
	if (!entry){
		log_enomem();
		return -1;
	}
	entry->key = sh_dup(key);
	entry->value_len = value_len;
	entry->value = malloc(value_len);
	memcpy(entry->value, value, value_len);

	if (!entry->key || !entry->value){
		log_enomem();
		free_opt_entry(entry);
		return -1;
	}

	if (write_entry(fp, entry) != 0){
		log_error("Failed to write entry to file");
		free_opt_entry(entry);
		return -1;
	}

	free_opt_entry(entry);
	return 0;
}

int read_option_file(const char* option_file, struct opt_entry*** out, size_t* len_out){
	FILE* fp = NULL;
	int ret = 0;
	struct opt_entry** arr = NULL;
	size_t arr_len = 0;
	size_t i;

	return_ifnull(option_file, -1);
	return_ifnull(out, -1);
	return_ifnull(len_out, -1);

	fp = fopen(option_file, "r");
	if (!fp){
		log_efopen(option_file);
		ret = -1;
		goto cleanup_freeout;
	}

	if (fscanf(fp, "[Options]\n") != 0){
		log_error("option_file is not the correct format");
		ret = -1;
		goto cleanup_freeout;
	}

	do{
		if (add_opt_entry(&arr, &arr_len, read_entry(fp)) != 0){
			log_error("Failed to push opt entry");
			ret = -1;
			goto cleanup_freeout;
		}
	}while (arr[arr_len - 1]);

	remove_null_entries(&arr, &arr_len);
	sort_opt_entries(arr, arr_len);

	fp ? fclose(fp) : 0;
	*out = arr;
	*len_out = arr_len;
	return ret;

cleanup_freeout:
	for (i = 0; i < arr_len; ++i){
		free_opt_entry(arr[i]);
	}
	free(arr);
	arr = NULL;
	arr_len = 0;
	fp ? fclose(fp) : 0;
	return ret;
}

void free_opt_entry_array(struct opt_entry** entries, size_t len){
	size_t i;
	if (!entries){
		return;
	}
	for (i = 0; i < len; ++i){
		free_opt_entry(entries[i]);
	}
	free(entries);
}

int binsearch_opt_entries(const struct opt_entry* const* entries, size_t len, const char* key){
	long left = 0;
	long right = len - 1;

	if (!entries || len <= 0){
		return -1;
	}

	while (left <= right){
		size_t mid = (left + right) >> 1;
		int cmp_val = strcmp(key, entries[mid]->key);
		if (cmp_val < 0){
			right = mid - 1;
		}
		else if (cmp_val > 0){
			left = mid + 1;
		}
		else{
			return mid;
		}
	}
	return -1;
}

/*
   int ssearch_opt_entries(struct opt_entry** entries, size_t len, const char* key){
   size_t i;
   for (i = 0; i < len; ++i){
   if (strcmp(entries[i]->key, key) == 0){
   return i;
   }
   }
   return -1;
   }
   */
