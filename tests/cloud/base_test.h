/** @file tests/cloud/base_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CLOUD_BASE_TEST_H
#define __CLOUD_BASE_TEST_H

#include "../test_framework.h"

void test_cloud_ui(enum TEST_STATUS* status);
void test_cloud(enum TEST_STATUS* status);

EXPORT_PKG(cloud_base_pkg);
#endif
