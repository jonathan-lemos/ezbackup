/** @file tests/strings/stringarray_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __STRINGARRAY_TEST_H
#define __STRINGARRAY_TEST_H

#include "../test_framework.h"

void test_sa_add(enum TEST_STATUS* status);
void test_sa_insert(enum TEST_STATUS* status);
void test_sa_contains(enum TEST_STATUS* status);
void test_sa_sanitize_directories(enum TEST_STATUS* status);
void test_sa_sort(enum TEST_STATUS* status);
void test_sa_cmp(enum TEST_STATUS* status);
void test_sa_get_parent_dirs(enum TEST_STATUS* status);
void test_sa_to_raw_array(enum TEST_STATUS* status);
void test_sa_merge(enum TEST_STATUS* status);

EXPORT_PKG(stringarray_pkg);
#endif
