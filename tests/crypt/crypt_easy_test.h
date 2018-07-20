/** @file tests/crypt/crypt_easy_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CRYPT_EASY_TEST_H
#define __CRYPT_EASY_TEST_H

#include "../test_framework.h"

void test_easy_encrypt(enum TEST_STATUS* status);
void test_easy_encrypt_inplace(enum TEST_STATUS* status);

EXPORT_PKG(crypt_easy_pkg);
#endif
