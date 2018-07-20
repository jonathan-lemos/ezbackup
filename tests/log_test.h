/** @file tests/log_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __TEST_LOG_H
#define __TEST_LOG_H

#include "test_framework.h"

void test_return_ifnull(enum TEST_STATUS* status);
void test_log(enum TEST_STATUS* status);

extern const struct test_pkg log_pkg;
#endif
