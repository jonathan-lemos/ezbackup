/* test_fileiterator.c -- tests fileiterator.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "fileiterator_test.h"
#include "../fileiterator.h"
#include "../log.h"
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

	fis = fi_start("/");
	TEST_ASSERT(fis);

	while ((tmp = fi_next(fis)) != NULL){
		if (strlen(tmp) <= 60){
			printf("%s", tmp);
		}
		else{
			printf("%.60s...", tmp);
		}
		printf(
				"          "\
				"          "\
				"          "\
				"          "\
				"          "\
				"          "\
				"   \r");
		TEST_FREE(tmp, free);
	}
	printf("\n");

cleanup:
	fis ? fi_end(fis) : (void)0;
	free(tmp);
}

void test_fi_skip_dir(enum TEST_STATUS* status){
	struct fi_stack* fis = NULL;
	char* tmp = NULL;
	int ctr = 0;

	fis = fi_start("/");
	TEST_ASSERT(fis);

	while ((tmp = fi_next(fis)) != NULL){
		if (strlen(tmp) <= 60){
			printf("%s", tmp);
		}
		else{
			printf("%.60s...", tmp);
		}
		printf(
				"          "\
				"          "\
				"          "\
				"          "\
				"          "\
				"          "\
				"   \r");
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
	free(tmp);
}

void test_fi_fail(enum TEST_STATUS* status){
	struct fi_stack* fis;

	fis = fi_start("/not/a/directory");
	TEST_ASSERT(fis == NULL);

cleanup:
	;
}
