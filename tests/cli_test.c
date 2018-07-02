/* cli_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "test_base.h"
#include "../log.h"
#include "../cli.h"
#include <stdlib.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

void test_display_dialog(enum TEST_STATUS* status){
	const char* choices[] = {
		"Abort",
		"Retry",
		"Fail"
	};
	int res;

	TEST_ASSERT((res = display_dialog(choices, ARRAY_SIZE(choices), "C\nC-C\nCOMBO BREAKER")) >= 0);
	switch(res){
		case 0:
			printf("Abort chosen\n");
			break;
		case 1:
			printf("Retry chosen\n");
			break;
		case 2:
			printf("Fail chosen\n");
			break;
	}
	TEST_ASSERT(pause_yn("Is the statement above correct (Y/N)?") == 0);
cleanup:
	;
}

void test_display_menu(enum TEST_STATUS* status){
	char** choices = NULL;
	int res;
	int i;

	choices = calloc(50, sizeof(*choices));
	TEST_ASSERT(choices);
	for (i = 0; i < 50; ++i){
		choices[i] = calloc(sizeof("Choice 00"), 1);
		TEST_ASSERT(choices[i]);

		sprintf(choices[i], "Choice %02d", i + 1);
	}

	TEST_ASSERT((res = display_menu((const char* const*)choices, 50, "Menu Test")) >= 0);
	printf("Choice %02d chosen\n", res + 1);

	TEST_ASSERT(pause_yn("Is the statement above correct (Y/N)?") == 0);

cleanup:
	for (i = 0; i < 50; ++i){
		free(choices[i]);
	}
	free(choices);
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_display_dialog),
		MAKE_TEST(test_display_menu)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
