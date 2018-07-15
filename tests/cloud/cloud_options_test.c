#include "../test_base.h"
#include "../../cloud/cloud_options.h"
#include "../../log.h"
#include <string.h>

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

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_co)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
