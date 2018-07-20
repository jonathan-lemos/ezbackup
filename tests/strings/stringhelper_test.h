/** @file tests/strings/stringhelper_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __STRINGHELPER_TEST_H
#define __STRINGHELPER_TEST_H

#include "../test_framework.h"

void test_sh_dup(enum TEST_STATUS* status);
void test_sh_concat(enum TEST_STATUS* status);
void test_sh_concat_path(enum TEST_STATUS* status);
void test_sh_filename(enum TEST_STATUS* status);
void test_sh_file_ext(enum TEST_STATUS* status);
void test_sh_starts_with(enum TEST_STATUS* status);
void test_sh_getcwd(enum TEST_STATUS* status);
void test_sh_cmp_nullsafe(enum TEST_STATUS* status);
void test_sh_sprintf(enum TEST_STATUS* status);

EXPORT_PKG(stringhelper_pkg);
#endif
