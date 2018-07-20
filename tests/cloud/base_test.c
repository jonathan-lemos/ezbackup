/** @file tests/cloud/base_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "base_test.h"
#include "../../cloud/base.h"
#include "../../cloud/keys.h"
#include "../../log.h"
#include <stdlib.h>
#include <string.h>

const struct unit_test cloud_base_tests[] = {
	MAKE_TEST_RU(test_cloud_ui),
	MAKE_TEST(test_cloud)
};
MAKE_PKG(cloud_base_tests, cloud_base_pkg);

const char* CLOUD_BASE_DIR = "/test";

static struct cloud_options* get_co_options(void){
	struct cloud_options* co = co_new();

	if (!co){
		return NULL;
	}

	co_set_cp(co, CLOUD_MEGA);
	if (co->cp != CLOUD_MEGA){
		co_free(co);
		return NULL;
	}
	co_set_username(co, MEGA_SAMPLE_USERNAME);
	if (strcmp(co->username, MEGA_SAMPLE_USERNAME) != 0){
		co_free(co);
		return NULL;
	}
	co_set_password(co, MEGA_SAMPLE_PASSWORD);
	if (strcmp(co->password, MEGA_SAMPLE_PASSWORD) != 0){
		co_free(co);
		return NULL;
	}
	co_set_upload_directory(co, CLOUD_BASE_DIR);
	if (strcmp(co->upload_directory, CLOUD_BASE_DIR) != 0){
		co_free(co);
		return NULL;
	}

	return co;
}

void test_cloud_ui(enum TEST_STATUS* status){
	struct cloud_options* co = get_co_options();
	struct cloud_data* cd = NULL;
	char* dir_mk = NULL;
	char* tmp = NULL;
	const char* file = "equifax_passwords.txt";
	unsigned char data[1337];

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));

	TEST_ASSERT(co);
	TEST_ASSERT(cloud_login(co, &cd) == 0);
	TEST_ASSERT(cloud_mkdir_ui(CLOUD_BASE_DIR, &dir_mk, cd) == 0);
	TEST_ASSERT(cloud_stat(dir_mk, NULL, cd) == 0);
	TEST_ASSERT(cloud_upload_ui(file, dir_mk, &tmp, cd) == 0);
	remove(file);
	TEST_ASSERT(cloud_download_ui(dir_mk, (char**)&file, cd) == 0);
	TEST_ASSERT(cloud_remove_ui(dir_mk, NULL, cd) == 0);
	TEST_ASSERT(memcmp_file_data(file, data, sizeof(data)) == 0);
	TEST_ASSERT_FREE(cd, cloud_logout);

cleanup:
	cd ? cloud_logout(cd) : 0;
	remove(file);
	free(dir_mk);
	free(tmp);
	co_free(co);
}

void test_cloud(enum TEST_STATUS* status){
	struct cloud_options* co = get_co_options();
	struct cloud_data* cd = NULL;
	const char* file = "equifax_passwords.txt";
	const char* upload_dir = "/test1/test2";
	const char* upload_file = "/test1/test2/equifax_passwords.txt";
	unsigned char data[1337];

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));

	TEST_ASSERT(co);
	TEST_ASSERT(cloud_login(co, &cd) == 0);
	TEST_ASSERT(cloud_upload(file, upload_dir, cd) == 0);
	remove(file);
	TEST_ASSERT(cloud_download(upload_file, (char**)&file, cd) == 0);
	TEST_ASSERT(cloud_remove(upload_file, cd) == 0);
	TEST_ASSERT(memcmp_file_data(file, data, sizeof(data)) == 0);
	TEST_ASSERT_FREE(cd, cloud_logout);

cleanup:
	remove(file);
	cd ? cloud_logout(cd) : 0;
	co_free(co);
}
