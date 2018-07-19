/* coredumps_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "coredumps_test.h"
#include "../coredumps.h"
#include "../log.h"
#include <sys/resource.h>

const struct unit_test coredumps_tests[] = {
	MAKE_TEST(test_coredumps)
};
MAKE_PKG(coredumps_tests, coredumps_pkg);

void test_coredumps(enum TEST_STATUS* status){
	struct rlimit rl;

	disable_core_dumps();
	TEST_ASSERT(getrlimit(RLIMIT_CORE, &rl) == 0);
	TEST_ASSERT(rl.rlim_max == 0 && rl.rlim_cur == 0);

	enable_core_dumps();
	TEST_ASSERT(getrlimit(RLIMIT_CORE, &rl) == 0);
	TEST_ASSERT(rl.rlim_max > 0 && rl.rlim_cur > 0);

cleanup:
	;
}
