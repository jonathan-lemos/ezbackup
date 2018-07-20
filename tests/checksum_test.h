/** @file tests/checksum_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __TEST_CHECKSUM_H
#define __TEST_CHECKSUM_H

#include "test_framework.h"

void test_bytes_to_hex(enum TEST_STATUS* status);
void test_checksum(enum TEST_STATUS* status);
void test_sort_checksum_file(enum TEST_STATUS* status);
void test_search_for_checksum(enum TEST_STATUS* status);
void test_create_removed_list(enum TEST_STATUS* status);

EXPORT_PKG(checksum_pkg);
#endif
