/* stringarray_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "../test_base.h"
#include "../../strings/stringarray.h"
#include "../../strings/stringhelper.h"
#include "../../log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

void test_sa_add(enum TEST_STATUS* status){
	struct string_array* sa = NULL;

	sa = sa_new();
	TEST_ASSERT(sa);
	TEST_ASSERT(sa_add(sa, "hunter2") == 0);
	TEST_ASSERT(sa_add(sa, "letmein") == 0);
	TEST_ASSERT(sa->len == 2);
	TEST_ASSERT(strcmp(sa->strings[0], "hunter2") == 0);
	TEST_ASSERT(strcmp(sa->strings[1], "letmein") == 0);
	TEST_ASSERT(sa_remove(sa, 0) == 0);
	TEST_ASSERT(sa->len == 1);
	TEST_ASSERT(strcmp(sa->strings[0], "letmein") == 0);

cleanup:
	sa ? sa_free(sa) : (void)0;
}

void test_sa_contains(enum TEST_STATUS* status){
	struct string_array* sa = NULL;

	sa = sa_new();
	TEST_ASSERT(sa);
	TEST_ASSERT(sa_add(sa, "hunter2") == 0);
	TEST_ASSERT(sa_add(sa, "letmein") == 0);
	TEST_ASSERT(sa->len == 2);
	TEST_ASSERT(sa_contains(sa, "hunter2"));
	TEST_ASSERT(sa_contains(sa, "letmein"));
	TEST_ASSERT(sa_remove(sa, 0) == 0);
	TEST_ASSERT(sa_contains(sa, "letmein"));
	TEST_ASSERT(!sa_contains(sa, "hunter2"));

cleanup:
	sa ? sa_free(sa) : (void)0;
}

void test_sa_sanitize_directories(enum TEST_STATUS* status){
	struct string_array* sa = NULL;
	char* cwd = NULL;
	char* cwd_subdir = NULL;

	cwd = sh_getcwd();
	TEST_ASSERT(cwd);

	cwd_subdir = make_path(2, cwd, "newdir");
	TEST_ASSERT(cwd_subdir);
	TEST_ASSERT(mkdir(cwd_subdir, 0755) == 0 || errno == EEXIST);

	sa = sa_new();
	TEST_ASSERT(sa);

	TEST_ASSERT(sa_add(sa, "/dev") == 0);
	TEST_ASSERT(sa_add(sa, "/home/equifax/passwords.txt") == 0);
	TEST_ASSERT(sa_add(sa, cwd) == 0);
	TEST_ASSERT(sa_add(sa, cwd_subdir) == 0);

	TEST_ASSERT(sa_sanitize_directories(sa) == 1);
	TEST_ASSERT(sa_contains(sa, "/dev"));
	TEST_ASSERT(sa_contains(sa, cwd));
	TEST_ASSERT(sa_contains(sa, cwd_subdir));
	TEST_ASSERT(!sa_contains(sa, "/home/equifax/passwords.txt"));

	rmdir(cwd_subdir);
	TEST_ASSERT(sa_sanitize_directories(sa) == 1);
	TEST_ASSERT(sa_contains(sa, "/dev"));
	TEST_ASSERT(sa_contains(sa, cwd));
	TEST_ASSERT(!sa_contains(sa, cwd_subdir));
	TEST_ASSERT(!sa_contains(sa, "/home/equifax/passwords.txt"))

cleanup:
	free(cwd);
	cwd_subdir ? rmdir(cwd_subdir) : 0;
	free(cwd_subdir);
	sa ? sa_free(sa) : (void)0;
}

void test_sa_sort(enum TEST_STATUS* status){
	struct string_array* sa = NULL;

	sa = sa_new();
	TEST_ASSERT(sa);

	TEST_ASSERT(sa_add(sa, "apple") == 0);
	TEST_ASSERT(sa_add(sa, "banana") == 0);
	TEST_ASSERT(sa_add(sa, "cthulhu") == 0);
	TEST_ASSERT(sa_add(sa, "avocado") == 0);

	sa_sort(sa);
	TEST_ASSERT(sa->len == 4);
	TEST_ASSERT(strcmp(sa->strings[0], "apple") == 0);
	TEST_ASSERT(strcmp(sa->strings[1], "avocado") == 0);
	TEST_ASSERT(strcmp(sa->strings[2], "banana") == 0);
	TEST_ASSERT(strcmp(sa->strings[3], "cthulhu") == 0);

cleanup:
	sa ? sa_free(sa) : (void)0;
}

void test_sa_cmp(enum TEST_STATUS* status){
	struct string_array* sa = NULL;
	struct string_array* sa_pass = NULL;
	struct string_array* sa_fail1 = NULL;
	struct string_array* sa_fail2 = NULL;

	sa = sa_new();
	sa_pass = sa_new();
	sa_fail1 = sa_new();
	sa_fail2 = sa_new();

	TEST_ASSERT(sa && sa_pass && sa_fail1 && sa_fail2);

cleanup:
	sa ? sa_free(sa) : (void)0;
	sa_pass ? sa_free(sa_pass) : (void)0;
	sa_fail1 ? sa_free(sa_fail1) : (void)0;
	sa_fail2 ? sa_free(sa_fail2) : (void)0;
}

void test_sa_get_parent_dirs(enum TEST_STATUS* status){
	const char* base_str = "/dir1/dir2/dir3";
	const char* match1 = "/dir1";
	const char* match2 = "/dir1/dir2";
	const char* match3 = "/dir1/dir2/dir3";
	struct string_array* sa = NULL;

	sa = sa_get_parent_dirs(base_str);
	TEST_ASSERT(sa);

	TEST_ASSERT(strcmp(sa->strings[0], match1) == 0);
	TEST_ASSERT(strcmp(sa->strings[1], match2) == 0);
	TEST_ASSERT(strcmp(sa->strings[2], match3) == 0);

cleanup:
	sa_free(sa);
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_sa_add),
		MAKE_TEST(test_sa_contains),
		MAKE_TEST(test_sa_sanitize_directories),
		MAKE_TEST(test_sa_sort),
		MAKE_TEST(test_sa_cmp),
		MAKE_TEST(test_sa_get_parent_dirs)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
