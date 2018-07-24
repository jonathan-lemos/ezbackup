/** @file tests/options/options_menu_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "options_menu_test.h"
#include "../../options/options_menu.h"
#include "../../log.h"

const struct unit_test options_menu_tests[] = {
	MAKE_TEST_RU(test_menu_operation),
	MAKE_TEST_RU(test_menu_configure)
};
MAKE_PKG(options_menu_tests, options_menu_pkg);

void test_menu_operation(enum TEST_STATUS* status){
	enum operation op;
	op = menu_operation();

	eprintf_default("You chose %s\n", operation_tostring(op));
	TEST_ASSERT(pause_yn("Is the above output correct (Y/N)?") == 0);
cleanup:
	;
}

void test_menu_configure(enum TEST_STATUS* status){
	struct options* opt = options_new();

	TEST_ASSERT(menu_configure(opt) == 0);
	TEST_ASSERT(write_options_tofile("/dev/stdout", opt) == 0);
	TEST_ASSERT(pause_yn("Is the above output correct (Y/N)?") == 0);
cleanup:
	opt ? options_free(opt) : (void)0;
}
