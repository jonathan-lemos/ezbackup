/** @file tests/filehelper_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __FILEHELPER_TEST_H
#define __FILEHELPER_TEST_H

#include "test_framework.h"

void test_read_file(enum TEST_STATUS* status);
void test_temp_fopen(enum TEST_STATUS* status);
void test_file_opened_for_reading(enum TEST_STATUS* status);
void test_file_opened_for_writing(enum TEST_STATUS* status);
void test_copy_file(enum TEST_STATUS* status);
void test_rename_file(enum TEST_STATUS* status);
void test_exists(enum TEST_STATUS* status);

extern const struct test_pkg filehelper_pkg;
#endif
