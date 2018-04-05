/* test_base.h -- helper functions for tests
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __TEST_BASE_H
#define __TEST_BASE_H

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

void printf_red(const char* format, ...);
void printf_yellow(const char* format, ...);
void printf_green(const char* format, ...);
void printf_blue(const char* format, ...);
void set_signal_handler(void);
void create_file(const char* name, const unsigned char* data, int len);
int memcmp_file_data(const char* file, const unsigned char* data, int data_len);
int memcmp_file_file(const char* file1, const char* file2);
void __massert(int condition, const char* file, int line, const char* msg);
#define massertm(condition, msg) __massert((intptr_t)(condition), __FILE__, __LINE__, msg)
#define massert(condition) __massert((intptr_t)(condition), __FILE__, __LINE__, #condition)

#endif
