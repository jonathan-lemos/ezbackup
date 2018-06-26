/* options_file_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "../test_base.h"
#include "../../log.h"
#include "../../options/options_file.h"
#include <string.h>
#include <stdint.h>

const char* const sample_data_8be =
"[Options]\n"
"KEY1=\0\0\0\0\0\0\0\x06hello\0\n"
"KEY2=\0\0\0\0\0\0\0\x06world\0\n";
const size_t size_8 =
sizeof("[Options]\n") - 1 +
sizeof("KEY1=\0\0\0\0\0\0\0\x06hello\0\n") - 1 +
sizeof("KEY2=\0\0\0\0\0\0\0\x06world\0\n") - 1;

const char* const sample_data_4be =
"[Options]\n"
"KEY1=\0\0\0\x06hello\0\n"
"KEY2=\0\0\0\x06world\0\n";
const size_t size_4 =
sizeof("[Options]\n") - 1 +
sizeof("KEY1=\0\0\0\x06hello\0\n") - 1 +
sizeof("KEY2=\0\0\0\x06world\0\n") - 1;

const char* const sample_data_8le =
"[Options]\n"
"KEY1=\x06\0\0\0\0\0\0\0hello\0\n"
"KEY2=\x06\0\0\0\0\0\0\0world\0\n";

const char* const sample_data_4le =
"[Options]\n"
"KEY1=\x06\0\0\0hello\0\n"
"KEY2=\x06\0\0\0world\0\n";

static int is_little_endian(void){
	uint32_t test = 0x01020304;
	unsigned char* ptr = (unsigned char*)&test;
	return ptr[0] == 0x04;
}

void test_add_option_tofile(enum TEST_STATUS* status){
	const char* data;
	size_t size;
	const char* path = "test.txt";
	FILE* fp_options = NULL;

	CT_ASSERT(sizeof(size_t) == 8 || sizeof(size_t) == 4);

	if (sizeof(size_t) == 8){
		size = size_8;
		data = is_little_endian() ? sample_data_8le : sample_data_8be;
	}
	else{
		size = size_4;
		data = is_little_endian() ? sample_data_4le : sample_data_4be;
	}

	fp_options = create_option_file(path);
	TEST_ASSERT(fp_options);

	TEST_ASSERT(add_option_tofile(fp_options, "KEY1", "hello", sizeof("hello")) == 0);
	TEST_ASSERT(add_option_tofile(fp_options, "KEY2", "world", sizeof("world")) == 0);

	TEST_ASSERT_FREE(fp_options, fclose);

	TEST_ASSERT(memcmp_file_data(path, (const unsigned char*)data, size) == 0);

cleanup:
	fp_options ? fclose(fp_options) : 0;
	remove(path);
}

void test_read_option_file(enum TEST_STATUS* status){
	const char* path = "test.txt";
	struct opt_entry** entries = NULL;
	size_t entries_len = 0;
	const char* data;
	size_t size;
	int res;

	CT_ASSERT(sizeof(size_t) == 8 || sizeof(size_t) == 4);

	if (sizeof(size_t) == 8){
		size = size_8;
		data = is_little_endian() ? sample_data_8le : sample_data_8be;
	}
	else{
		size = size_4;
		data = is_little_endian() ? sample_data_4le : sample_data_4be;
	}

	create_file(path, data, size);
	TEST_ASSERT(read_option_file(path, &entries, &entries_len) == 0);

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "KEY1");
	TEST_ASSERT(res >= 0);
	TEST_ASSERT(memcmp(entries[res]->value, "hello", sizeof("hello")) == 0);
	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "KEY2");
	TEST_ASSERT(res >= 0);
	TEST_ASSERT(memcmp(entries[res]->value, "world", sizeof("world")) == 0);

cleanup:
	free_opt_entry_array(entries, entries_len);
	remove(path);
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_add_option_tofile),
		MAKE_TEST(test_read_option_file)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
