/** @file tests/fileiterator_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __TEST_FILEITERATOR_H
#define __TEST_FILEITERATOR_H

#include "test_framework.h"

void test_fi_normal(enum TEST_STATUS* status);
void test_fi_skip_dir(enum TEST_STATUS* status);
void test_fi_fail(enum TEST_STATUS* status);

extern const struct test_pkg fileiterator_pkg;
#endif
