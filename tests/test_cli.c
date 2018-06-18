#include "test_base.h"
#include "../log.h"
#include "../cli.h"
#include <stdlib.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

void test_display_dialog(void){
	const char* choices[] = {
		"Abort",
		"Retry",
		"Fail"
	};
	int res;

	printf_blue("Testing display_dialog()\n");
	massert((res = display_dialog(choices, ARRAY_SIZE(choices), "C\nC-C\nCOMBO BREAKER")) >= 0);
	switch(res){
		case 0:
			printf_yellow("Abort chosen\n");
			break;
		case 1:
			printf_yellow("Retry chosen\n");
			break;
		case 2:
			printf_yellow("Fail chosen\n");
			break;
	}

	printf_green("Finished testing display_dialog()\n\n");
}

void test_display_menu(void){
	char** choices;
	int res;
	int i;

	choices = malloc(sizeof(*choices) * 50);
	massert(choices);
	for (i = 0; i < 50; ++i){
		choices[i] = malloc(sizeof("Choice 00"));
		massert(choices[i]);

		sprintf(choices[i], "Choice %02d", i + 1);
	}

	massert((res = display_menu((const char* const*)choices, 50, "Menu Test")) >= 0);
	printf_yellow("Choice %02d chosen\n", i);

	printf_green("Finished testing display_menu()\n\n");
}

int main(void){
	set_signal_handler();
	log_setlevel(LEVEL_INFO);

	test_display_dialog();
	test_display_menu();

	return 0;
}
