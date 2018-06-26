/* options_file.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __OPTIONS_FILE_H
#define __OPTIONS_FILE_H

#include <stdio.h>
#include <stddef.h>

struct opt_entry{
	char* key;
	void* value;
	size_t value_len;
};

FILE* create_option_file(const char* path);
int add_option_tofile(FILE* fp, const char* key, const void* value, size_t value_len);
int read_option_file(const char* option_file, struct opt_entry*** out, size_t* len_out);
int binsearch_opt_entries(const struct opt_entry* const* entries, size_t len, const char* key);
void free_opt_entry_array(struct opt_entry** entries, size_t len);

#endif
