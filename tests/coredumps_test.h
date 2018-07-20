/** @file tests/coredumps_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __COREDUMPS_TEST_H
#define __COREDUMPS_TEST_H

#include "test_framework.h"

void test_coredumps(enum TEST_STATUS* status);

extern const struct test_pkg coredumps_pkg;
#endif
