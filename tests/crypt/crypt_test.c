/** @file tests/crypt/crypt_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "crypt_test.h"
#include "../../crypt/crypt.h"
#include "../../log.h"
#include <stdlib.h>
#include <string.h>

static const char* const sample_file = "crypt.txt";
static const char* const sample_file_crypt = "crypt.txt.aes";
static const char* const sample_file_crypt2 = "crypt2.txt.aes";
static const char* const sample_file_decrypt = "decrypt.txt";
static const char* const sample_file_decrypt2 = "decrypt2.txt";
static const char* const password = "password";
static const unsigned char salt[8] = {0, 0, 0, 0, 0, 0, 0, 0};

const struct unit_test crypt_tests[] = {
	MAKE_TEST(test_crypt_encrypt),
	MAKE_TEST(test_crypt_decrypt)
};
MAKE_PKG(crypt_tests, crypt_pkg);

void test_crypt_encrypt(enum TEST_STATUS* status){
	struct crypt_keys* fk = NULL;
	char openssl_cmd[256];
	unsigned char sample_data[999];

	fill_sample_data(sample_data, sizeof(sample_data));
	create_file(sample_file, sample_data, sizeof(sample_data));

	sprintf(openssl_cmd, "openssl aes-256-cbc -e -S 0000000000000000 -in %s -out %s -pass pass:%s", sample_file, sample_file_crypt, password);
	printf("%s\n", openssl_cmd);
	system(openssl_cmd);

	TEST_ASSERT((fk = crypt_new()) != NULL);
	TEST_ASSERT(crypt_set_encryption(EVP_aes_256_cbc(), fk) == 0);
	TEST_ASSERT(crypt_set_salt(salt, fk) == 0);
	TEST_ASSERT(crypt_gen_keys((const unsigned char*)password, strlen(password), NULL, 1, fk) == 0);
	TEST_ASSERT(crypt_encrypt(sample_file, fk, sample_file_crypt2) == 0);

	TEST_ASSERT(memcmp_file_file(sample_file_crypt, sample_file_crypt2) == 0);

cleanup:
	fk ? crypt_free(fk) : (void)0;
	remove(sample_file);
	remove(sample_file_crypt);
	remove(sample_file_crypt2);
}

void test_crypt_decrypt(enum TEST_STATUS* status){
	struct crypt_keys* fk = NULL;
	char openssl_cmd[256];
	unsigned char sample_data[999];

	fill_sample_data(sample_data, sizeof(sample_data));

	create_file(sample_file, sample_data, sizeof(sample_data));

	sprintf(openssl_cmd, "openssl aes-256-cbc -e -S 0000000000000000 -in %s -out %s -pass pass:%s", sample_file, sample_file_crypt, password);
	printf("%s\n", openssl_cmd);
	system(openssl_cmd);

	sprintf(openssl_cmd, "openssl aes-256-cbc -d -salt -in %s -out %s -pass pass:%s", sample_file_crypt, sample_file_decrypt, password);
	printf("%s\n", openssl_cmd);
	system(openssl_cmd);
	TEST_ASSERT(memcmp_file_data(sample_file_decrypt, sample_data, sizeof(sample_data)) == 0);

	TEST_ASSERT((fk = crypt_new()) != NULL);
	TEST_ASSERT(crypt_set_encryption(EVP_aes_256_cbc(), fk) == 0);
	TEST_ASSERT(crypt_extract_salt(sample_file_crypt, fk) == 0);
	TEST_ASSERT(crypt_gen_keys((const unsigned char*)password, strlen(password), NULL, 1, fk) == 0);
	TEST_ASSERT(crypt_decrypt(sample_file_crypt, fk, sample_file_decrypt2) == 0);

	TEST_ASSERT(memcmp_file_file(sample_file_decrypt, sample_file_decrypt2) == 0);

cleanup:
	fk ? crypt_free(fk) : (void)0;
	remove(sample_file);
	remove(sample_file_crypt);
	remove(sample_file_decrypt);
	remove(sample_file_decrypt2);
}
