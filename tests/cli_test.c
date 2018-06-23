#include "test_base.h"
#include "../log.h"
#include "../cli.h"
#include <stdlib.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

int test_display_dialog(void){
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

	return pause_yn("Is the statement above correct (Y/N)?");
}

int test_display_menu(void){
	char** choices;
	int res;
	int i;

	choices = malloc(sizeof(*choices) * 50);
	TEST_ASSERT(choices);
	for (i = 0; i < 50; ++i){
		choices[i] = malloc(sizeof("Choice 00"));
		TEST_ASSERT(choices[i]);

		sprintf(choices[i], "Choice %02d", i + 1);
	}

	TEST_ASSERT((res = display_menu((const char* const*)choices, 50, "Menu Test")) >= 0);
	printf("Choice %02d chosen\n", i);

	return pause_yn("Is the statement above correct (Y/N)?");
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
