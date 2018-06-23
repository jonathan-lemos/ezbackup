#include "../test_base.h"
#include "../../cloud/include.h"
#include "../../cloud/keys.h"
#include "../../log.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

void test_co(enum TEST_STATUS* status){
	struct cloud_options* co;

	co = co_new();
	TEST_ASSERT(co);

	TEST_ASSERT(co_set_username(co, "bobby_basketball") == 0);
	TEST_ASSERT(strcmp("bobby_basketball", co->username) == 0);
	TEST_ASSERT(co_set_username(co, NULL) == 0);
	TEST_ASSERT(co->username = NULL);
	TEST_ASSERT(co_set_username(co, "bobby_basketball") == 0);
	TEST_ASSERT(strcmp("bobby_basketball", co->username) == 0);

	TEST_ASSERT(co_set_password(co, "hunter2") == 0);
	TEST_ASSERT(strcmp("hunter2", co->password));
	TEST_ASSERT(co_set_password(co, NULL) == 0);
	TEST_ASSERT(co->password = NULL);
	TEST_ASSERT(co_set_password(co, "hunter2") == 0);
	TEST_ASSERT(strcmp("hunter2", co->password) == 0);


	TEST_ASSERT(co_set_default_upload_directory(co) == 0);
	TEST_ASSERT(co->upload_directory);
	printf("Default upload dir: %s\n", co->upload_directory);
	TEST_ASSERT(co_set_upload_directory(co, NULL) == 0);
	TEST_ASSERT(co->upload_directory = NULL);
	TEST_ASSERT(co_set_upload_directory(co, "/dir1/dir2") == 0);
	TEST_ASSERT(strcmp("/dir1/dir2", co->upload_directory) == 0);

cleanup:
	co ? co_free(co) : (void)0;
}

void test_time_menu(enum TEST_STATUS* status){
	int res;
	/* Jan 1 1970 GMT */
	struct file_node node0 = {"file1.txt", 0};
	/* Jan 1 1980 GMT */
	struct file_node node1 = {"file2.txt", 315532800};
	/* Jan 1 2000 GMT */
	struct file_node node2 = {"file3.txt", 946684800};
	struct file_node* arr[3];

	arr[0] = &node0;
	arr[1] = &node1;
	arr[2] = &node2;

	res = time_menu(arr, sizeof(arr) / sizeof(arr[0]));

	printf("You selected %d\n", res);
	TEST_ASSERT(pause_yn("Is the above statement correct (Y/N)?") == 0);

cleanup:
	;
}

void test_get_parent_dirs(enum TEST_STATUS* status){
	const char* dir = "/dir1/dir2/dir3";
	char** out = NULL;
	size_t out_len = 0;
	size_t i;

	TEST_ASSERT(get_parent_dirs(dir, &out, &out_len) == 0);

	TEST_ASSERT(out_len == 3);
	TEST_ASSERT(strcmp(out[0], "/dir1") == 0);
	TEST_ASSERT(strcmp(out[1], "/dir1/dir2") == 0);
	TEST_ASSERT(strcmp(out[2], "/dir1/dir2/dir3") == 0);

cleanup:
	for (i = 0; i < out_len; ++i){
		free(out[i]);
	}
	free(out);
}

void test_cloud_download(enum TEST_STATUS* status){
	const char* file = "file1.txt";
	const char* data = "xyzzy";
	const char* dir_base = "/test1";
	const char* dir = "/test1/test2";
	const char* full_path = "/test1/test2/file1.txt";
	char* file_out = "file2.txt";
	struct cloud_options* co = NULL;

	create_file(file, (const unsigned char*)data, strlen(data));
	co = co_new();
	co_set_username(co, MEGA_SAMPLE_USERNAME);
	co_set_password(co, MEGA_SAMPLE_PASSWORD);
	co_set_cp(co, CLOUD_MEGA);
	co_set_upload_directory(co, dir);

	TEST_ASSERT(cloud_upload(file, co) == 0);

	remove(file);
	TEST_ASSERT(cloud_download(dir, co, &file_out) == 0);

	TEST_ASSERT(memcmp_file_data(file_out, (const unsigned char*)data, strlen(data)) == 0);

	TEST_ASSERT(cloud_rm(full_path, co) == 0);
	TEST_ASSERT(cloud_rm(dir, co) == 0);
	TEST_ASSERT(cloud_rm(dir_base, co) == 0);

cleanup:
	co_free(co);
	remove(file_out);
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_co),
		MAKE_TEST(test_time_menu),
		MAKE_TEST(test_get_parent_dirs),
		MAKE_TEST(test_cloud_download)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
