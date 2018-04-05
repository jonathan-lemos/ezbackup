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

void test_display_menu(void){
	char** options;
	int i;
	int res;

	printf_blue("Testing display_menu()\n");

	options = malloc(sizeof(*options) * 100);
	massert(options);
	for (i = 0; i < 100; ++i){
		char buf[64];
		sprintf(buf, "Choice %d", i);
		options[i] = malloc(strlen(buf) + 1);
		massert(options[i]);
		strcpy(options[i], buf);
	}
	res = display_menu((const char**)options, 100, "Test Menu");
	massert(res >= 0);
	printf("Option %d selected\n", res + 1);
	mpause();
}

void test_write_options_tofile(void){
	printf_blue("Testing write_options_tofile()\n");

}

int main(void){
	set_signal_handler();

	test_version_usage();
	return 0;
}
