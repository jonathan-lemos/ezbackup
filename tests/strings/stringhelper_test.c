/* stringhelper_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "../test_base.h"
#include "../../strings/stringhelper.h"
#include "../../log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_sh_dup(enum TEST_STATUS* status){
	const char* test_str = "hunter2";
	char* tmp = NULL;

	tmp = sh_dup(test_str);
	TEST_ASSERT(tmp);
	TEST_ASSERT(strcmp(tmp, test_str) == 0);

cleanup:
	free(tmp);
}

void test_sh_concat(enum TEST_STATUS* status){
	const char* target_str = "hello world";
	char* test = NULL;

	test = sh_dup("hello ");
	TEST_ASSERT(test);
	test = sh_concat(test, "world");
	TEST_ASSERT(test);
	TEST_ASSERT(strcmp(target_str, test) == 0);

cleanup:
	free(test);
}

void test_sh_concat_path(enum TEST_STATUS* status){
	const char* target_str = "dir1/dir2/file.txt";
	const char* dir1 = "dir1";
	const char* sdir1 = "dir1/";
	const char* dir2 = "dir2";
	const char* sdir2 = "/dir2/";
	const char* file = "file.txt";
	const char* sfile = "/file.txt";

	char* tmp;

	tmp = sh_concat_path(sh_dup(dir1), dir2);
	tmp = sh_concat_path(tmp, file);
	TEST_ASSERT(strcmp(tmp, target_str) == 0);
	TEST_FREE(tmp, free);

	tmp = sh_concat_path(sh_concat_path(sh_dup(sdir1), dir2), sfile);
	TEST_ASSERT(strcmp(tmp, target_str) == 0);
	TEST_FREE(tmp, free);

	tmp = sh_dup(sdir1);
	tmp = sh_concat_path(tmp, sdir2);
	tmp = sh_concat_path(tmp, file);
	TEST_ASSERT(strcmp(tmp, target_str) == 0);
	TEST_FREE(tmp, free);

cleanup:
	free(tmp);
}

void test_sh_filename(enum TEST_STATUS* status){
	const char* test_path = "/home/equifax/passwords.txt";
	const char* target_filename = "passwords.txt";
	const char* test = NULL;

	test = sh_filename(test_path);
	TEST_ASSERT(test);
	TEST_ASSERT(strcmp(target_filename, test) == 0);
cleanup:
	;
}

void test_sh_file_ext(enum TEST_STATUS* status){
	const char* test_path = "/home/equifax/passwords.txt";
	const char* target_ext = "txt";
	const char* test = NULL;

	test = sh_file_ext(test_path);
	TEST_ASSERT(test);
	TEST_ASSERT(strcmp(target_ext, test) == 0);
cleanup:
	;
}

void test_sh_starts_with(enum TEST_STATUS* status){
	const char* test_str = "hunter2";
	const char* pass1 = "hunt";
	const char* pass2 = "hunter2";
	const char* fail1 = "hunger2";
	const char* fail2 = "hunter23";

	TEST_ASSERT(sh_starts_with(test_str, pass1));
	TEST_ASSERT(sh_starts_with(test_str, pass2));
	TEST_ASSERT(!sh_starts_with(test_str, fail1));
	TEST_ASSERT(!sh_starts_with(test_str, fail2));

cleanup:
	;
}

void test_sh_getcwd(enum TEST_STATUS* status){
	char* cwd = NULL;
	char* test = NULL;

	cwd = sh_getcwd();
	TEST_ASSERT(cwd);

	test = malloc(strlen(cwd) + 2);
	TEST_ASSERT(test);

	TEST_ASSERT(getcwd(test, strlen(cwd) + 2));
	TEST_ASSERT(strcmp(cwd, test) == 0);

cleanup:
	free(cwd);
	free(test);
}

void test_sh_cmp_nullsafe(enum TEST_STATUS* status){
	const char* test_str = "hunter2";
	const char* pass     = "hunter2";
	const char* fail1    = "hunger2";
	const char* fail2    = "hunter23";

	TEST_ASSERT(sh_cmp_nullsafe(test_str, pass) == 0);
	TEST_ASSERT(sh_cmp_nullsafe(test_str, fail1) > 0);
	TEST_ASSERT(sh_cmp_nullsafe(test_str, fail2) < 0);
	TEST_ASSERT(sh_cmp_nullsafe(test_str, NULL) < 0);
	TEST_ASSERT(sh_cmp_nullsafe(NULL, test_str) > 0);
	TEST_ASSERT(sh_cmp_nullsafe(NULL, NULL) == 0);

cleanup:
	;
}

void test_sh_sprintf(enum TEST_STATUS* status){
	char* buf = NULL;

	buf = sh_sprintf("Hello world");
	TEST_ASSERT(buf);
	TEST_ASSERT(strcmp(buf, "Hello world") == 0);
	TEST_FREE(buf, free);

	buf = sh_sprintf("Hello %s %d", "world", 15);
	TEST_ASSERT(buf);
	TEST_ASSERT(strcmp(buf, "Hello world 15") == 0);
	TEST_FREE(buf, free);

cleanup:
	free(buf);
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_sh_dup),
		MAKE_TEST(test_sh_concat),
		MAKE_TEST(test_sh_concat_path),
		MAKE_TEST(test_sh_filename),
		MAKE_TEST(test_sh_file_ext),
		MAKE_TEST(test_sh_starts_with),
		MAKE_TEST(test_sh_getcwd),
		MAKE_TEST(test_sh_cmp_nullsafe),
		MAKE_TEST(test_sh_sprintf)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
