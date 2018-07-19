/* cloud_options_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "cloud_options_test.h"
#include "../../cloud/cloud_options.h"
#include "../../log.h"
#include <string.h>

const struct unit_test cloud_options_tests[] = {
	MAKE_TEST(test_co)
};
MAKE_PKG(cloud_options_tests, cloud_options_pkg);

void test_co(enum TEST_STATUS* status){
	struct cloud_options* co;

	co = co_new();
	TEST_ASSERT(co);

	TEST_ASSERT(co_set_username(co, "bobby_basketball") == 0);
	TEST_ASSERT(strcmp("bobby_basketball", co->username) == 0);
	TEST_ASSERT(co_set_username(co, NULL) == 0);
	TEST_ASSERT(co->username == NULL);
	TEST_ASSERT(co_set_username(co, "bobby_basketball") == 0);
	TEST_ASSERT(strcmp("bobby_basketball", co->username) == 0);

	TEST_ASSERT(co_set_password(co, "hunter2") == 0);
	TEST_ASSERT(strcmp("hunter2", co->password) == 0);
	TEST_ASSERT(co_set_password(co, NULL) == 0);
	TEST_ASSERT(co->password == NULL);
	TEST_ASSERT(co_set_password(co, "hunter2") == 0);
	TEST_ASSERT(strcmp("hunter2", co->password) == 0);

	TEST_ASSERT(co_set_default_upload_directory(co) == 0);
	TEST_ASSERT(co->upload_directory);
	printf("Default upload dir: %s\n", co->upload_directory);
	TEST_ASSERT(co_set_upload_directory(co, NULL) == 0);
	TEST_ASSERT(co->upload_directory == NULL);
	TEST_ASSERT(co_set_upload_directory(co, "/dir1/dir2") == 0);
	TEST_ASSERT(strcmp("/dir1/dir2", co->upload_directory) == 0);

cleanup:
	co ? co_free(co) : (void)0;
}
