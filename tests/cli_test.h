/** @file tests/cli_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __TEST_CLI_H
#define __TEST_CLI_H

#include "test_framework.h"

void test_display_dialog(enum TEST_STATUS* status);
void test_display_menu(enum TEST_STATUS* status);

extern const struct test_pkg cli_pkg;
#endif
