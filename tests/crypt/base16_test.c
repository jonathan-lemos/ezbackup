/* base16_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "../test_base.h"
#include "../../crypt/base16.h"
#include "../../log.h"
#include <stdlib.h>
#include <string.h>

const char* const b16_str = "123456789ABCDEFF";
const unsigned char b16_bytes[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xFF};

void test_to_base16(enum TEST_STATUS* status){
	char* str = NULL;

	TEST_ASSERT(to_base16(b16_bytes, sizeof(b16_bytes), &str) == 0);
	TEST_ASSERT(strcmp(b16_str, str) == 0);

cleanup:
	free(str);
}

void test_from_base16(enum TEST_STATUS* status){
	unsigned char* bytes = NULL;
	unsigned out_len = 0;

	TEST_ASSERT(from_base16(b16_str, (void**)&bytes, &out_len) == 0);
	TEST_ASSERT(out_len == sizeof(b16_bytes));
	TEST_ASSERT(memcmp(b16_bytes, bytes, out_len) == 0);

cleanup:
	free(bytes);
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_to_base16),
		MAKE_TEST(test_from_base16)
	};
	log_setlevel(LEVEL_INFO);

	START_TESTS(tests);
	return 0;
}
