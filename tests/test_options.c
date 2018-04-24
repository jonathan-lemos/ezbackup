/* test_options.c -- tests options.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "test_base.h"
#include "../options.h"
#include <stdlib.h>
#include <string.h>

void mpause(void){
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
	printf("Press ENTER to continue...");
	getchar();
}

void test_version_usage(void){
	printf_blue("Testing version()/usage()\n");

	printf_yellow("Calling version()\n");
	version();

	printf_yellow("Calling usage()\n");
	usage("PROGRAM_NAME");

	printf_green("Finished testing version()/usage()\n\n");
}

void test_parse_options_cmdline(void){
	struct options opt;
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

	printf_blue("Testing parse_options_cmdline()\n");

	printf_yellow("Testing parse_options_cmdline() (pass)\n");
	massert(parse_options_cmdline(sizeof(argv_pass) / sizeof(argv_pass[0]), argv_pass, &opt) == 0);
	free_options(&opt);
	printf_yellow("Testing parse_options_cmdline() (fail)\n");
	massert(parse_options_cmdline(sizeof(argv_fail) / sizeof(argv_fail[0]), argv_fail, &opt) == 5);
	free_options(&opt);

	printf_green("Finished testing parse_options_cmdline()\n\n");
}

void test_parse_options_menu(void){
	struct options opt;

	printf_blue("Testing parse_options_menu()\n");

	printf_yellow("Calling parse_options_menu()\n");
	massert(get_default_options(&opt) == 0);
	massert(parse_options_menu(&opt) == 0);

	printf_yellow("Verify the following options are correct\n");
	massert(write_options_tofile("/dev/stdout", &opt) == 0);

	free_options(&opt);
	mpause();

	printf_green("Finished testing parse_options_menu()\n\n");
}

void test_parse_options_fromfile(void){
	struct options opt;
	struct options opt_read;

	printf_blue("Testing parse_options_fromfile()\n");

	printf_yellow("Creating an options file\n");
	massert(get_default_options(&opt) == 0);
	massert(write_options_tofile("options.txt", &opt) == 0);

	printf_yellow("Calling parse_options_fromfile()\n");
	memcpy(&opt_read, &opt, sizeof(opt_read));
	massert(parse_options_fromfile("options.txt", &opt_read) == 0);

	printf_yellow("Checking that it worked\n");
	massert(strcmp(opt.output_directory, opt_read.output_directory) == 0);
	free(opt_read.output_directory);
	opt_read.output_directory = opt.output_directory;
	massert(memcmp(&opt, &opt_read, sizeof(opt)) == 0);

	free_options(&opt);
	remove("options.txt");

	printf_green("Finished testing parse_options_fromfile()\n\n");
}

void test_read_config_file(void){
	struct options opt;
	struct options opt_read;

	printf_blue("Testing read_config_file()\n");

	printf_yellow("Calling write_config_file()\n");
	massert(get_default_options(&opt) == 0);
	massert(write_config_file(&opt, NULL) == 0);

	printf_yellow("Calling read_config_file()\n");
	memcpy(&opt_read, &opt, sizeof(opt));
	massert(read_config_file(&opt_read) == 0);

	printf_yellow("Checking that it worked\n");
	massert(strcmp(opt.output_directory, opt_read.output_directory) == 0);
	free(opt_read.output_directory);
	opt_read.output_directory = opt.output_directory;
	massert(memcmp(&opt, &opt_read, sizeof(opt)) == 0);

	free_options(&opt);

	printf_green("Finished testing read_config_file()\n\n");
}

void test_display_menu(void){
	char** options;
	int i;
	int res;

	printf_blue("Testing display_menu()\n");

	options = malloc(sizeof(*options) * 100);
	massert(options);
	for (i = 0; i < 100; ++i){
		char buf[64];
		sprintf(buf, "Choice %d", i + 1);
		options[i] = malloc(strlen(buf) + 1);
		massert(options[i]);
		strcpy(options[i], buf);
	}

	printf_yellow("Calling display_menu()\n");
	res = display_menu((const char**)options, 100, "Test Menu");
	massert(res >= 0);
	printf("Option %d selected\n", res + 1);
	mpause();

	for (i = 0; i < 100; ++i){
		free(options[i]);
	}
	free(options);

	printf_green("Finished testing display_menu()\n\n");
}

void test_write_options_tofile(void){
	struct options opt;

	printf_blue("Testing write_options_tofile()\n");

	printf_yellow("Calling get_default_options()\n");
	massert(get_default_options(&opt) == 0);

	printf_yellow("Calling write_options_tofile(stdout)\n");
	massert(write_options_tofile("/dev/stdout", &opt) == 0);

	free_options(&opt);
	mpause();

	printf_green("Finished testing write_options_tofile()\n\n");
}

int main(void){
	set_signal_handler();

	test_version_usage();
	test_parse_options_cmdline();
	test_parse_options_menu();
	test_parse_options_fromfile();
	test_read_config_file();
	test_display_menu();
	test_write_options_tofile();
	return 0;
}
