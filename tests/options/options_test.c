/** @file tests/options/options_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "options_test.h"
#include "../../log.h"
#include "../../options/options.h"
#include "../../strings/stringhelper.h"
#include <stdlib.h>
#include <string.h>

const struct unit_test options_tests[] = {
	MAKE_TEST(test_parse_options_cmdline),
	MAKE_TEST(test_parse_options_fromfile),
};
MAKE_PKG(options_tests, options_pkg);

static struct options* sample_options(void){
	struct options* opt;

	opt = options_new();
	if (!opt){
		eprintf_red("Failed to make new options object");
		return NULL;
	}

	opt->enc_algorithm = EVP_aes_192_ctr();
	opt->enc_password = sh_dup("hunter2");
	opt->cloud_options->username = sh_dup("azurediamond");
	sa_add(opt->directories, "/me/me");
	sa_add(opt->directories, "/dev/null");
	sa_add(opt->directories, "/home/azurediamond/passwords/hunter2");
	sa_add(opt->exclude, "/winblows/system32");

	return opt;
}

void test_version_usage(enum TEST_STATUS* status){
	version();
	usage("PROGRAM_NAME");
	TEST_ASSERT(pause_yn("Is the above output correct (Y/N)?") == 0);
cleanup:
	;
}

void test_parse_options_cmdline(enum TEST_STATUS* status){
	struct options* opt;
	enum operation op;
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

	/* TODO: make this test */
	return;

	TEST_ASSERT(parse_options_cmdline(sizeof(argv_pass) / sizeof(argv_pass[0]), argv_pass, &opt, &op) == 0);
	options_free(opt);
	TEST_ASSERT(parse_options_cmdline(sizeof(argv_fail) / sizeof(argv_fail[0]), argv_fail, &opt, &op) == 5);

cleanup:
	opt ? options_free(opt) : (void)0;
}

void test_parse_options_fromfile(enum TEST_STATUS* status){
	struct options* opt = NULL;
	struct options* opt_read = NULL;
	char* file = "options.txt";

	opt = sample_options();
	TEST_ASSERT(opt);

	TEST_ASSERT(write_options_tofile(file, opt) == 0);
	TEST_ASSERT(parse_options_fromfile(file, &opt_read) == 0);
	TEST_ASSERT(options_cmp(opt, opt_read) == 0);

cleanup:
	opt ? options_free(opt) : (void)0;
	opt_read ? options_free(opt_read) : (void)0;
	remove(file);
}
