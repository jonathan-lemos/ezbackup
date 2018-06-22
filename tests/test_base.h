/* test_base.h -- helper functions for tests
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __TEST_BASE_H
#define __TEST_BASE_H

#define __UNIT_TESTING__

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

void log_red(const char* format, ...);
void log_yellow(const char* format, ...);
void log_green(const char* format, ...);
void log_blue(const char* format, ...);

void set_signal_handler(void);

void create_file(const char* name, const unsigned char* data, int len);
int memcmp_file_data(const char* file, const unsigned char* data, int data_len);
int memcmp_file_file(const char* file1, const char* file2);
int does_file_exist(const char* file);
void setup_test_environment_basic(const char* path, char*** out, size_t* out_len);
void setup_test_environment_full(const char* path, char*** out, size_t* out_len);
void cleanup_test_environment(const char* path, char** files);

int test_assert(int condition, const char* file, int line, const char* msg);

struct unit_test{
	int(*func)(void);
	const char* func_name;
};
int run_tests(const struct unit_test* tests, size_t len);

#define TEST_SUCCESS 0
#define TEST_FAIL 1

#define TEST_ASSERT(condition) if (test_assert((intptr_t)(condition), __FILE__, __LINE__, #condition)) return TEST_FAIL
#define TEST_ASSERT_MSG(condition, msg) if (test_assert((intptr_t)(condition), __FILE__, __LINE__, msg)) return TEST_FAIL

#define MAKE_TEST(func) {func, #func}
#define START_TESTS(tests) set_signal_handler();run_tests(tests, sizeof(tests) / sizeof(tests[0]))

#endif
