/** @file tests/log_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "log_test.h"
#include "../log.h"

const struct unit_test log_tests[] = {
	MAKE_TEST(test_return_ifnull),
	MAKE_TEST_RU(test_log)
};
MAKE_PKG(log_tests, log_pkg);

static int func_return_ifnull(const void* arg){
	return_ifnull(arg, 1);
	return 0;
}

void test_return_ifnull(enum TEST_STATUS* status){
	const char* tmp = "444";
	TEST_ASSERT(func_return_ifnull(tmp) == 0);
	tmp = NULL;
	TEST_ASSERT(func_return_ifnull(tmp) != 0);
cleanup:
	;
}

void test_log(enum TEST_STATUS* status){
	log_setlevel(LEVEL_INFO);
	log_fatal("Fatal");
	log_error("Error");
	log_warning("Warning");
	log_debug("Debug");
	log_info("Info");

	eprintf_default("\n");

	log_setlevel(LEVEL_FATAL);
	log_fatal("Fatal");
	log_error("Error");
	log_warning("Warning");
	log_debug("Debug");
	log_info("Info");

	TEST_ASSERT(pause_yn("Last line is FATAL (Y/N)?") == 0);

cleanup:
	;
}
