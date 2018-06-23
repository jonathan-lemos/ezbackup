/* test_options.c -- tests options.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "../test_base.h"
#include "../../log.h"
#include "../../options/options.h"
#include <stdlib.h>
#include <string.h>

void test_version_usage(enum TEST_STATUS* status){
	version();
	usage("PROGRAM_NAME");
	TEST_ASSERT(pause_yn("Is the above output correct (Y/N)?") == 0);
cleanup:
	;
}

void test_parse_options_cmdline(enum TEST_STATUS* status){
	struct options* opt;
	enum OPERATION op;
	char* argv_pass[] = {
		"PROG_NAME",
		"backup",
		"-c",
		"bz2",
		"-C",
		"sha256",
		"-d",
		"/dir1",
		"/dir2",
		"-e",
		"aes-128-cbc",
		"-o",
		"/out.file",
		"-v",
		"-x",
		"/ex1",
		"/ex2",
		"-u",
		"john_doe@example.com",
		"-p",
		"hunter2",
		"-i",
		"mega",
		"-I",
		"/mybackups"
	};
	char* argv_fail[] = {
		"PROG_NAME",
		"-c",
		"bz2",
		"-C",
		"sha256",
		"-q",
		"-d",
		"/dir1",
		"/dir2",
		"-e",
		"aes-128-cbc",
		"-o",
		"/out.file",
		"-v",
		"-x",
		"/ex1",
		"/ex2"
	};


	TEST_ASSERT(parse_options_cmdline(sizeof(argv_pass) / sizeof(argv_pass[0]), argv_pass, &opt, &op) == 0);
	free_options(opt);
	TEST_ASSERT(parse_options_cmdline(sizeof(argv_fail) / sizeof(argv_fail[0]), argv_fail, &opt, &op) == 5);

cleanup:
	opt ? free_options(opt) : (void)0;
}

void test_parse_options_menu(enum TEST_STATUS* status){
	struct options* opt = NULL;

	TEST_ASSERT(parse_options_menu(&opt) == 0);
	TEST_ASSERT(write_options_tofile("/dev/stdout", opt) == 0);
	TEST_ASSERT(pause_yn("Is the above output correct (Y/N)?") == 0);

cleanup:
	opt ? free_options(opt) : (void)0;
}

void test_parse_options_fromfile(enum TEST_STATUS* status){
	struct options* opt = NULL;
	struct options* opt_read = NULL;
	char* file = "options.txt";

	TEST_ASSERT((opt = options_new()) != NULL);
	opt->output_directory = malloc(sizeof("/test/dir"));
	TEST_ASSERT(opt->output_directory);
	strcpy(opt->output_directory, "/test/dir");

	TEST_ASSERT(write_options_tofile(file, opt) == 0);

	memcpy(&opt_read, &opt, sizeof(opt_read));
	TEST_ASSERT(parse_options_fromfile(file, &opt_read) == 0);

	TEST_ASSERT(strcmp(opt->output_directory, opt_read->output_directory) == 0);

cleanup:
	opt ? free_options(opt) : (void)0;
	opt_read ? free_options(opt_read) : (void)0;
	remove(file);
}

void test_write_options_tofile(enum TEST_STATUS* status){
	struct options* opt = NULL;

	TEST_ASSERT((opt = options_new()) != NULL);
	TEST_ASSERT(write_options_tofile("/dev/stdout", opt) == 0);
	TEST_ASSERT(pause_yn("Is the above output correct (Y/N)?") == 0);

cleanup:
	opt ? free_options(opt) : (void)0;
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_version_usage),
		MAKE_TEST(test_parse_options_cmdline),
		MAKE_TEST(test_parse_options_menu),
		MAKE_TEST(test_parse_options_fromfile),
		MAKE_TEST(test_write_options_tofile)
	};
	log_setlevel(LEVEL_INFO);

	START_TESTS(tests);
	return 0;
}
