/** @file mtimefile/entry.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This softare may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "entry.h"
#include "../log.h"
#include <stdlib.h>

static size_t next_p2(size_t x){
	size_t c = 1;
	while (x >>= 1){
		c <<= 1;
	}
	return c << 1;
}

EZB_MALLOC_LIKE struct entry_array* entry_arr_new(void){
	struct entry_array* ret = malloc(sizeof(*ret));
	if (!ret){
		log_enomem();
		return NULL;
	}
	ret->size = 0;
	ret->capacity = ENTRY_ARR_DEFAULT_CAPACITY;
	ret->entries = malloc(sizeof(*(ret->entries)) * ret->capacity);
	if (!(ret->entries)){
		free(ret);
		log_enomem();
		return NULL;
	}
	return ret;
}

static int entry_arr_resize(struct entry_array* ea, size_t needed_capacity){
	size_t cap_new = next_p2(needed_capacity);
	void* tmp;

	if (cap_new == ea->capacity){
		return 0;
	}

	tmp = realloc(ea->entries, sizeof(*(ea->entries)) * cap_new);
	if (!tmp){
		log_enomem();
		return -1;
	}
	ea->entries = tmp;
	ea->capacity = cap_new;
	return 0;
}

int entry_arr_add(struct entry_array* ea, struct entry* e){
	if (entry_arr_resize(ea, ea->capacity + 1) != 0){
		log_error("Failed to resize entry array");
		return -1;
	}

	ea->entries[ea->size] = e;
	ea->size++;
	return 0;
}

void entry_arr_free(struct entry_array* ea){
	size_t i;
	if (!ea){
		return;
	}
	for (i = 0; i < ea->size; ++i){
		free(ea->entries[i]);
	}
	free(ea->entries);
	free(ea);
}
