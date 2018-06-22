/* test_fileiterator.c -- tests fileiterator.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "test_base.h"
#include "../fileiterator.h"
#include "../log.h"
#include <stdlib.h>

int test_fi_normal(void){
	struct fi_stack* fis;
	char* tmp;

	fis = fi_start("/");
	TEST_ASSERT(fis);

	while ((tmp = fi_next(fis)) != NULL){
		printf("%s\r", tmp);
		free(tmp);
	}
	printf("\n");

	fi_end(fis);
	return TEST_SUCCESS;
}

int test_fi_skip_dir(void){
	struct fi_stack* fis;
	char* tmp;
	int ctr = 0;

	fis = fi_start("/");
	TEST_ASSERT(fis);

	while ((tmp = fi_next(fis)) != NULL){
		printf("%s\r", tmp);
		free(tmp);

		ctr++;
		if (ctr >= 3){
			fi_skip_current_dir(fis);
			ctr = 0;
		}
	}
	printf("\n");

	fi_end(fis);
	return TEST_SUCCESS;
}

int test_fi_fail(void){
	struct fi_stack* fis;

	fis = fi_start("/not/a/directory");
	TEST_ASSERT(fis == NULL);

	return TEST_SUCCESS;
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_fi_normal),
		MAKE_TEST(test_fi_skip_dir),
		MAKE_TEST(test_fi_fail)
	};

	log_setlevel(LEVEL_INFO);

	START_TESTS(tests);
	return 0;
}
