/** @file tests/crypt/crypt_easy_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "crypt_easy_test.h"
#include "../../crypt/crypt_easy.h"
#include "../../log.h"
#include <stdlib.h>
#include <string.h>

const struct unit_test crypt_easy_tests[] = {
	MAKE_TEST(test_easy_encrypt),
	MAKE_TEST(test_easy_encrypt_inplace),
};
MAKE_PKG(crypt_easy_tests, crypt_easy_pkg);

void test_easy_encrypt(enum TEST_STATUS* status){
	const char* file = "file.txt";
	const char* file_crypt = "file_crypt.txt";
	const char* file_decrypt = "file_decrypt.txt";
	unsigned char data[1337];

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));

	TEST_ASSERT(easy_encrypt(file, file_crypt, "AES-256-CBC", 1, "hunter2") == 0);
	TEST_ASSERT(easy_decrypt(file_crypt, file_decrypt, "AES-256-CBC", 1, "hunter2") == 0);
	TEST_ASSERT(memcmp_file_file(file, file_decrypt) == 0);

cleanup:
	remove(file);
	remove(file_crypt);
	remove(file_decrypt);
}

void test_easy_encrypt_inplace(enum TEST_STATUS* status){
	const char* file = "file.txt";
	unsigned char data[1337];

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));

	TEST_ASSERT(easy_encrypt_inplace(file, "AES-256-CBC", 1, "hunter2") == 0);
	TEST_ASSERT(easy_decrypt_inplace(file, "AES-256-CBC", 1, "hunter2") == 0);

cleanup:
	remove(file);
}
