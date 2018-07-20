/** @file tests/options/options_file_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __OPTIONS_FILE_TEST_H
#define __OPTIONS_FILE_TEST_H

#include "../test_framework.h"

void test_add_option_tofile(enum TEST_STATUS* status);
void test_read_option_file(enum TEST_STATUS* status);

EXPORT_PKG(options_file_pkg);
#endif
