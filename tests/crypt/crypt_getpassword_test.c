/** @file tests/crypt/crypt_getpassword_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "crypt_getpassword_test.h"
#include "../../crypt/crypt_getpassword.h"
#include "../../log.h"

const struct unit_test crypt_getpassword_tests[] = {
	MAKE_TEST_RU(test_crypt_getpassword)
};
MAKE_PKG(crypt_getpassword_tests, crypt_getpassword_pkg);

void test_crypt_getpassword(enum TEST_STATUS* status){
	char* pw = NULL;
	int res;

	do{
		res = crypt_getpassword("Enter password:", "Verify password:", &pw);
	}while (res > 0);
	TEST_ASSERT(res == 0);

	printf("You entered %s\n", pw);
	TEST_ASSERT(pause_yn("Is this correct (Y/N)?") == 0);

	TEST_FREE(pw, crypt_freepassword);

cleanup:
	pw ? crypt_freepassword(pw) : (void)0;
}
