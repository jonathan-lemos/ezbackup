/* test_error.c -- tests error.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "test_base.h"
#include "../error.h"

int main(void){
	enum LOG_LEVEL level;

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

	return 0;
}
