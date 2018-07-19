/* progressbar_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "progressbar_test.h"
#include "../progressbar.h"
#include "../log.h"
#include <unistd.h>
#include <stdlib.h>

const struct unit_test progressbar_tests[] = {
	MAKE_TEST_RU(test_progress)
};
MAKE_PKG(progressbar_tests, progressbar_pkg);

void test_progress(enum TEST_STATUS* status){
	struct progress* p = NULL;

	srand(0);

	p = start_progress("Test progress", 100000);
	TEST_ASSERT(p);
	while (p->count < p->max){
		usleep(rand() % 150 + 10);
		inc_progress(p, 6);
	}
cleanup:
	p ? finish_progress(p) : (void)0;
}
