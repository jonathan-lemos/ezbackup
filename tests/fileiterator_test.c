/** @file tests/fileiterator_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "fileiterator_test.h"
#include "../fileiterator.h"
#include "../log.h"
#include "../readline_include.h"
#include <stdlib.h>
#include <string.h>

const struct unit_test fileiterator_tests[] = {
	MAKE_TEST_RU(test_fi_normal),
	MAKE_TEST_RU(test_fi_skip_dir),
	MAKE_TEST_RU(test_fi_fail)
};
MAKE_PKG(fileiterator_tests, fileiterator_pkg);

void test_fi_normal(enum TEST_STATUS* status){
	struct fi_stack* fis = NULL;
	char* tmp = NULL;
	char* dir = NULL;

	dir = readline("Enter directory:");

	fis = fi_start(dir);
	TEST_ASSERT(fis);

	while ((tmp = fi_next(fis)) != NULL){
		printf("%s\n", tmp);
		TEST_FREE(tmp, free);
	}
	printf("\n");

cleanup:
	fis ? fi_end(fis) : (void)0;
	free(dir);
	free(tmp);
}

void test_fi_skip_dir(enum TEST_STATUS* status){
	struct fi_stack* fis = NULL;
	char* tmp = NULL;
	char* dir = NULL;
	int ctr = 0;

	dir = readline("Enter directory:");

	fis = fi_start(dir);
	TEST_ASSERT(fis);

	while ((tmp = fi_next(fis)) != NULL){
		printf("%s\n", tmp);
		TEST_FREE(tmp, free);

		ctr++;
		if (ctr >= 3){
			fi_skip_current_dir(fis);
			ctr = 0;
		}
	}
	printf("\n");

cleanup:
	fis ? fi_end(fis) : (void)0;
	free(dir);
	free(tmp);
}

void test_fi_fail(enum TEST_STATUS* status){
	struct fi_stack* fis;

	fis = fi_start("/not/a/directory");
	TEST_ASSERT(fis == NULL);

cleanup:
	;
}
