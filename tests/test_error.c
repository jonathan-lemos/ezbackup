/* test_error.c -- tests error.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "test_base.h"
#include "../error.h"

int test_return_ifnull(char* arg){
	return_ifnull(arg, -1);
	return 0;
}

int main(void){
	enum LOG_LEVEL level;
	char* tmp = "hello";

	set_signal_handler();

	for (level = LEVEL_NONE; level <= LEVEL_INFO; level++){
		log_setlevel(level);
		log_info("info");
		log_debug("debug");
		log_warning("warning");
		log_error("error");
		log_fatal("fatal");
		printf("\n");
	}

	massert(test_return_ifnull(tmp) == 0);
	tmp = NULL;
	massert(test_return_ifnull(tmp) == -1);

	return 0;
}
