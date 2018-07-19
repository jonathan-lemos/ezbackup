/* zip_test.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __COMPRESSION_ZIP_TEST_H
#define __COMPRESSION_ZIP_TEST_H

#include "../test_framework.h"

void test_compress_gzip(enum TEST_STATUS* status);
void test_compress_bzip2(enum TEST_STATUS* status);
void test_compress_xz(enum TEST_STATUS* status);
void test_compress_lz4(enum TEST_STATUS* status);
void test_decompress_gzip(enum TEST_STATUS* status);
void test_decompress_bzip2(enum TEST_STATUS* status);
void test_decompress_xz(enum TEST_STATUS* status);
void test_decompress_lz4(enum TEST_STATUS* status);

EXPORT_PKG(compression_zip_pkg);
#endif
