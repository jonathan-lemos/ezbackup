/* crypt_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "../test_base.h"
#include "../../crypt/crypt_getpassword.h"
#include "../../log.h"

void test_crypt_getpassword(enum TEST_STATUS* status){
	char* pw = NULL;
	int res;

	do{
		res = crypt_getpassword("Enter password:", "Verify password:", &pw);
	}while (res > 0);
	TEST_ASSERT(res == 0);

	printf("You entered %s\n", pw);
	TEST_ASSERT(pause_yn("Is this correct (Y/N)?") == 0);

	TEST_ASSERT_FREE(pw, crypt_freepassword);

cleanup:
	pw ? crypt_freepassword(pw) : 0;
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_crypt_getpassword)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
