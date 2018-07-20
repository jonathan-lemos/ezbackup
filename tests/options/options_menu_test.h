/** @file tests/options/options_menu_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __OPTIONS_MENU_TEST_H
#define __OPTIONS_MENU_TEST_H

#include "../test_framework.h"

void test_menu_operation(enum TEST_STATUS* status);
void test_menu_configure(enum TEST_STATUS* status);

EXPORT_PKG(options_menu_pkg);
#endif
