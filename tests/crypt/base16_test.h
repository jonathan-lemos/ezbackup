/** @file tests/crypt/base16_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CRYPT_B16_TEST_H
#define __CRYPT_B16_TEST_H

#include "../test_framework.h"

void test_to_base16(enum TEST_STATUS* status);
void test_from_base16(enum TEST_STATUS* status);

EXPORT_PKG(crypt_base16_pkg);
#endif
