/** @file databuffer.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "databuffer.h"
#include "../log.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Returns the smallest power of 2 greater than a number.
 *
 * @param x The number.
 *
 * @return The smallest power of 2 greater than x.
 */
static size_t next_p2(size_t x){
	size_t c = 1;
	while (x >>= 1){
		c <<= 1;
	}
	return c << 1;
}

struct databuffer* db_create(void){
	struct databuffer* ret = calloc(1, sizeof(*ret));
	if (!ret){
		log_enomem();
		return NULL;
	}
	ret->capacity = DB_DEFAULT_CAPACITY;
	ret->data = malloc(DB_DEFAULT_CAPACITY);
	if (!(ret->data)){
		free(ret);
		log_enomem();
		return NULL;
	}
	return ret;
}

int db_fill(struct databuffer* db){
	db->size = 0;
	db->capacity = DB_DEFAULT_CAPACITY;
	db->data = malloc(DB_DEFAULT_CAPACITY);
	if (!(db->data)){
		log_enomem();
		return -1;
	}
	return 0;
}

int db_resize(struct databuffer* db, size_t needed_capacity){
	size_t cap_new = next_p2(needed_capacity);
	void* tmp;

	if (cap_new == db->capacity){
		return 0;
	}

	tmp = realloc(db->data, cap_new);
	if (!tmp){
		log_enomem();
		return -1;
	}
	db->data = tmp;
	db->capacity = cap_new;
	return 0;
}

int db_concat(struct databuffer* db, unsigned char* data, size_t len){
	if (db_resize(db, db->size + len) != 0){
		log_error("Failed to resize data buffer");
		return -1;
	}
	memcpy(db->data + db->size, data, len);
	db->size += len;
	return 0;
}

int db_concat_char(struct databuffer* db, unsigned char c){
	return db_concat(db, &c, 1);
}

void db_free_contents(struct databuffer* db){
	free(db->data);
}

void db_free(struct databuffer* db){
	free(db->data);
	free(db);
}
