#include "../test_base.h"
#include "../../cloud/include.h"
#include "../../log.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

void test_co(enum TEST_STATUS* status){
	struct cloud_options* co;

	printf_blue("Testing co_* functions\n");

	printf_yellow("Calling co_new()\n");
	co = co_new();
	massert(co);

	printf_yellow("Calling co_set_username(\"bobby_basketball\")\n");
	massert(co_set_username(co, "bobby_basketball") == 0);
	massert(strcmp("bobby_basketball", co->username) == 0);
	massert(co_set_username(co, NULL) == 0);
	massert(co->username = NULL);
	massert(co_set_username(co, "bobby_basketball") == 0);
	massert(strcmp("bobby_basketball", co->username) == 0);

	printf_yellow("Calling co_set_password(\"hunter2\")");
	massert(co_set_password(co, "hunter2") == 0);
	massert(strcmp("hunter2", co->password));
	massert(co_set_password(co, NULL) == 0);
	massert(co->password = NULL);
	massert(co_set_password(co, "hunter2") == 0);
	massert(strcmp("hunter2", co->password) == 0);

	printf_yellow("Calling co_set_default_upload_directory()\n");
	massert(co_set_default_upload_directory(co) == 0);
	massert(strcmp(co->upload_directory, "/Backups") == 0);
	massert(co_set_upload_directory(co, NULL) == 0);
	massert(co->upload_directory = NULL);
	massert(co_set_upload_directory(co, "/dir1/dir2") == 0);
	massert(strcmp("/dir1/dir2", co->upload_directory) == 0);

	printf_yellow("Calling co_free()\n");
	co_free(co);

	printf_green("Finished testing co_* functions\n");
}

void test_time_menu(enum TEST_STATUS* status){
	int res;
	/* Jan 1 1970 GMT */
	struct file_node node0 = {"file1.txt", 0};
	/* Jan 1 1980 GMT */
	struct file_node node1 = {"file2.txt", 315532800};
	/* Jan 1 2000 GMT */
	struct file_node node2 = {"file3.txt", 946684800};
	const struct file_node* arr[3];

	arr[0] = &node0;
	arr[1] = &node1;
	arr[2] = &node2;

	printf_blue("Testing time_menu()\n");

	printf_yellow("Calling time_menu()\n");
	res = time_menu(arr, sizeof(arr) / sizeof(arr[0]));

	printf("You selected %d\n", res);

	printf_green("Finished testing time_menu()\n\n");
}

void test_get_parent_dirs(enum TEST_STATUS* status){
	const char* dir = "/dir1/dir2/dir3";
	char** out = NULL;
	size_t out_len = 0;
	size_t i;

	printf_blue("Testing get_parent_dirs()\n");

	printf_yellow("Calling get_parent_dirs()\n");
	massert(get_parent_dirs(dir, &out, &out_len) == 0);

	printf_yellow("Checking that it worked\n");
	massert(out_len == 3);
	massert(strcmp(out[0], "/dir1") == 0);
	massert(strcmp(out[1], "/dir1/dir2") == 0);
	massert(strcmp(out[2], "/dir1/dir2/dir3") == 0);

	for (i = 0; i < out_len; ++i){
		free(out[i]);
	}
	free(out);

	printf_green("Finished testing get_parent_dirs()\n");
}

void test_cloud_download(enum TEST_STATUS* status){
	const char* file = "file1.txt";
	const char* data = "xyzzy";
	const char* dir_base = "/test1";
	const char* dir = "/test1/test2";
	const char* full_path = "/test1/test2/file1.txt";
	char* file_out;
	struct cloud_options* co;

	printf_blue("Testing cloud_upload()\n");

	create_file(file, (const unsigned char*)data, strlen(data));
	co = co_new();
	co_set_username(co, "***REMOVED***");
	co_set_password(co, "***REMOVED***");
	co_set_cp(co, CLOUD_MEGA);

	printf_yellow("Calling cloud_upload()\n");
	massert(cloud_upload(file, co) == 0);

	printf_yellow("Calling cloud_download()\n");
	remove(file);
	massert(cloud_download(dir, co, &file_out) == 0);

	printf_yellow("Checking that the files match\n");
	massert(memcmp_file_data(file_out, (const unsigned char*) data, strlen(data)) == 0);

	printf_yellow("Cleaning up\n");
	massert(cloud_rm(full_path, co) == 0);
	massert(cloud_rm(dir, co) == 0);
	massert(cloud_rm(dir_base, co) == 0);

	remove(file_out);
	printf_green("Finished testing cloud_download()\n\n");
}

int main(void){
	return 0;
}
