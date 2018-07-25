/** @file tests/test_framework.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __TEST_FRAMEWORK_H
#define __TEST_FRAMEWORK_H

#define __UNIT_TESTING__

#define ENABLE_BRIGHT_COLORS

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

/**
 * @brief Status code for the tests.
 */
enum TEST_STATUS{
	TEST_SUCCESS = 0, /**< @brief The test succeeded. */
	TEST_FAIL = 1     /**< @brief The test failed. */
};

/**
 * @brief Logs a message to stderr in red.<br>
 * If stderr does not point to a tty, the color is not used.
 *
 * @param format A printf format string.
 *
 * @return void
 */
void eprintf_red(const char* format, ...);

/**
 * @brief Logs a message to stderr in yellow.<br>
 * If stderr does not point to a tty, the color is not used.
 *
 * @param format A printf format string.
 *
 * @return void
 */
void eprintf_yellow(const char* format, ...);

/**
 * @brief Logs a message to stderr in green.<br>
 * If stderr does not point to a tty, the color is not used.
 *
 * @param format A printf format string.
 *
 * @return void
 */
void eprintf_green(const char* format, ...);

/**
 * @brief Logs a message to stderr in blue.<br>
 * If stderr does not point to a tty, the color is not used.
 *
 * @param format A printf format string.
 *
 * @return void
 */
void eprintf_blue(const char* format, ...);

/**
 * @brief Logs a message to stderr.
 *
 * @param format A printf format string.
 *
 * @return void
 */
void eprintf_default(const char* format, ...);

/**
 * @brief Creates a file with the specified path and data.<br>
 * An internal error is thrown and the program exits if there is an error.<br>
 * A corresponding message is displayed to the user.
 *
 * @param name The path of the file.<br>
 * If a file already exists at this path, it will be overwritten.
 *
 * @param data The data to fill the file with.
 *
 * @param len The length of the data in bytes.
 *
 * @return void
 */
void create_file(const char* name, const void* data, int len);

/**
 * @brief Compares the contents of a file and a block of data.
 *
 * @param file The path to the file.
 *
 * @param data The data.
 *
 * @param data_len The length of the data in bytes.
 *
 * @return 0 if the contents of the file and data are equivalent, negative if the first character that doesn't match is less in the file, positive if it is greater.<br>
 * This function will return negative if the length of the file is less than the data, and positive if the length of the file is greater than the data.
 */
int memcmp_file_data(const char* file, const void* data, int data_len);

/**
 * @brief Compares the contents of two files.
 *
 * @param file1 The path to the first file.
 *
 * @param file2 The path to the second file.
 *
 * @return 0 if the contents of the two files are equivalent, negative if the first character that doesn't match is less in file1, positive if it is greater.<br>
 * This function will return negative if the length of file1 is less than file2, and positive if it is greater.
 */
int memcmp_file_file(const char* file1, const char* file2);

/**
 * @brief Checks if a file exists.
 *
 * @param file1 The path to check the existence for.
 *
 * @return Positive if a file exists at that path, 0 if not.
 */
int does_file_exist(const char* file);

/**
 * @brief Sets up a basic test environment.<br>
 * It will have the following structure:<br>
 * <pre>
 * path (0755)
 *     path/file_{00-20}.txt (0666)
 * </pre>
 * <br>
 * This environment should be cleaned up with cleanup_test_environment() when it is no longer needed.
 * @see cleanup_test_environment().
 *
 * @param path The path to create the test environment under.
 *
 * @param out A pointer to a string array that will contain the filenames created.<br>
 * This can be NULL if it is not needed.
 *
 * @param out_len A pointer to an integer that will contain the length of the output array.<br>
 * This can be NULL if it is not needed.
 *
 * @return void
 */
void setup_test_environment_basic(const char* path, char*** out, size_t* out_len);

/**
 * @brief Sets up a complex test environment.<br>
 * It will have the following structure:<br>
 * <pre>
 * path (0755)
 * 		path/dir1 (0755)
 * 			path/dir1/d1file_{00-11}.txt (0666)
 * 		path/dir2 (0755)
 * 			path/dir2/d2file_{00-10}.txt (0666)
 * 		path/excl (0755)
 * 			path/excl/exfile_{00-09}.txt (0666)
 * 			path/excl/exfile_noacc.txt (0000)
 * 		path/noaccess (0000)
 * </pre>
 * <br>
 * This environment should be cleaned up with cleanup_test_environment() when it is no longer needed.
 * @see cleanup_test_environment().
 *
 * @param path The path to create the test environment under.
 *
 * @param out A pointer to a string array that will contain the filenames created.
 * This can be NULL if it is not needed.
 *
 * @param out_len A pointer to an integer that will contain the length of the output array.
 * This can be NULL if it is not needed.
 *
 * @return void
 */
void setup_test_environment_full(const char* path, char*** out, size_t* out_len);

/**
 * Cleans up any test environment created with a previous call to setup_test_environment_basic()/setup_test_environment_full()
 * @see setup_test_environment_basic()
 * @see setup_test_environment_full()
 *
 * @param path The path of the previously generated test environment.
 *
 * @param files The filename array generated by a previous call to setup_test_environment_*()<br>
 * This parameter can be NULL. All this function does is free the filename array if it exists.
 */
void cleanup_test_environment(const char* path, char** files);

/**
 * @brief Asks the user a yes/no question.
 *
 * @param prompt The prompt to ask the user. This should be terminated with a ':', but does not have to be.
 *
 * @return 0 if the user entered 'Y' or 'y', negative if not.
 */
int pause_yn(const char* prompt);

/**
 * @brief Fills a buffer with sample data.<br>
 * This data will have a cyclic "01234567890123..." pattern.
 *
 * @param ptr The data to fill.
 *
 * @param len The length of the data to fill in bytes.
 *
 * @return void
 */
void fill_sample_data(unsigned char* ptr, size_t len);

/**
 * @brief Makes a path out of a variable number of components.<br>
 * Examples:<br>
 * ```C
 * make_path(3, "/home", "equifax", "passwords.txt")    -> "/home/equifax/passwords.txt"
 * make_path(3, "/home/", "/equifax/", "passwords.txt") -> "/home/equifax/passwords.txt"
 * make_path(3, "home", "/equifax/", "/passwords.txt")  -> "home/equifax/passwords.txt"
 * ```
 *
 * @param n_components The number of components.
 *
 * @return A string containing the created path.<br>
 * This string must be free()'d when no longer in use.
 */
char* make_path(int n_components, ...);

/**
 * @brief Returns 0 if the condition is true. Otherwise prints an "Assertion Failed" message and returns non-zero.<br>
 * Do not call this function directly; use the TEST_ASSERT() macro.
 * @see TEST_ASSERT()
 *
 * @param condition The condition to verify.
 *
 * @param file The name of the file the assertion is being called from.<br>
 * This argument should be filled with the __FILE__ macro.
 *
 * @param line The line number the assertion is being called from.<br>
 * This argument should be filled with the __LINE__ macro.
 *
 * @param msg The assertion message to display on failure.
 *
 * @return 0 if the condition is true, or non-zero if false.
 */
int test_assert(intptr_t condition, const char* file, int line, const char* msg);

/**
 * @brief Does a compile-time assertion. The expression must be evaluated at compile-time.<br>
 * This expression works because if the condition is true, then sizeof(char[1 - 0]) is a valid expression.<br>
 * However, if the condition is false, then sizeof(char[1 - 2]) is invalid because an array size cannot be below zero.
 *
 * @param condition The condition to verify.
 */
#define CT_ASSERT(condition) ((void)sizeof(char[1 - 2 * !(condition)]))

/**
 * @brief Does a runtime assertion.<br>
 * If the condition is false, a message is displayed to the user showing the file, line number, and the failing condition as a string. The test immediately starts cleaning up afterwards.<br>
 * This expression can only be called within a function that matches the prototype void(*)(enum TEST_STATUS status).<br>
 * This parameter must be named "status"<br>
 * There also must be a label "cleanup" that can be goto'd.<br>
 * <br>
 * These are necessary to allow the test to report failure while also cleaning up any dynamically allocated resources before exiting.<br>
 *
 * @param condition The condition to verify.
 */
#define TEST_ASSERT(condition) if (test_assert((intptr_t)(condition), __FILE__, __LINE__, #condition)) {*status = TEST_FAIL;goto cleanup;}

/**
 * @brief Does a runtime assertion and displays a custom message.<br>
 * If the condition is false, a message is displayed to the user showing the file, line number, and a custom message. The test immediately starts cleaning up afterwards.<br>
 * This expression can only be called within a function that matches the prototype void(*)(enum TEST_STATUS status).<br>
 * This parameter must be named "status"<br>
 * There also must be a label "cleanup" that can be goto'd.<br>
 * <br>
 * These are necessary to allow the test to report failure while also cleaning up any dynamically allocated resources before exiting.<br>
 *
 * @param condition The condition to verify.
 *
 * @param msg The message to display.<br>
 * Use TEST_ASSERT() to display the condition as the message.
 * @see TEST_ASSERT()
 */
#define TEST_ASSERT_MSG(condition, msg) if (test_assert((intptr_t)(condition), __FILE__, __LINE__, msg)) {*status = TEST_FAIL;goto cleanup;}

/**
 * @brief Frees a resource and sets it to NULL.<br>
 * This is necessary to prevent double free problems arising from the cleanup label.<br>
 * Example usage:<br>
 * ```C
 * FILE* fp = fopen("foobar.txt", "wb");
 * //Do things with the file.
 * TEST_FREE(fp, fclose)
 * //fp is now NULL.
 * ```
 *
 * @param ptr The resource to free.
 *
 * @param free_func The function used to free.<br>
 * This function must take the resource to free as its sole parameter.
 *
 * @return ptr is now NULL.
 */
#define TEST_FREE(ptr, free_func) { free_func(ptr); ptr = NULL; }

/**
 * @brief Frees a resource and sets it to NULL on success or failure. On failure it fails a test assertion.<br>
 * This is necessary to prevent double free problems arising from the cleanup label.<br>
 * If the free function's return value does not equal zero, a message is displayed to the user showing the file, line number, and the free function that failed. The test immediately starts cleaning up afterwards.<br>
 * This expression can only be called within a function that matches the prototype void(*)(enum TEST_STATUS status).<br>
 * This parameter must be named "status"<br>
 * There also must be a label "cleanup" that can be goto'd.<br>
 * <br>
 * These are necessary to allow the test to report failure while also cleaning up any dynamically allocated resources before exiting.<br>
 *
 * @param ptr The resource to free.
 *
 * @param free_func The function used to free.<br>
 * The free function must return 0 on success and non-zero on failure.
 *
 * @return ptr is set to NULL regardless of if the free function succeeds. On failure, the function fails a test assertion.
 * @see TEST_ASSERT()
 */
#define TEST_ASSERT_FREE(ptr, free_func) if (free_func(ptr) != 0){ ptr = NULL; test_assert(0, __FILE__, __LINE__, #free_func);*status = TEST_FAIL;goto cleanup; } else { ptr = NULL; }

/**
 * @brief A single unit test.<br>
 * Do not initialize these directly. Use the MAKE_TEST() macro.
 *
 * @see MAKE_TEST()
 */
struct unit_test{
	/**
	 * @brief The unit test's function.<br>
	 * This function must fill the following prototype:<br>
	 * void(*)(enum TEST_STATUS* status)<br>
	 * The parameter must be named "status".<br>
	 * <br>
	 * Do not edit the value this parameter points to directly; use the TEST_ASSERT()/TEST_ASSERT_MSG() macros to indicate failure.<br>
	 * This value will be TEST_SUCCESS by default, so no additional action is needed to indicate success.
	 * @see TEST_ASSERT()
	 * @see TEST_ASSERT_MSG()
	 *
	 * If you want to use the TEST_ASSERT()/TEST_ASSERT_MSG() macros, there must be a label "cleanup" that can be goto'd.<br>
	 * The statements under this label should clean up all dynamic resources that may be allocated throughout the function.
	 */
	void(*func)(enum TEST_STATUS* status);
	const char* func_name;  /**< The name of the function. */
	unsigned requires_user; /**< True if the test requires user input, false if not. */
};

/**
 * @brief Turns a unit test function into a struct unit_test that does not require user input.<br>
 * This is designed to be used with a struct unit_test array like below:<br>
 * ```C
 * const struct unit_test tests[] = {
 *     MAKE_TEST(test_func1),
 *     MAKE_TEST(test_func2)
 * };
 * ```
 *
 * @param func The function to create a unit test out of.
 */
#define MAKE_TEST(func) {func, #func, 0}

/**
 * @brief Turns a unit test function into a struct unit_test that does require user input.<br>
 * This is designed to be used with a struct unit_test array like below:<br>
 * ```C
 * const struct unit_test tests[] = {
 *     MAKE_TEST_RU(test_requires_user1),
 *     MAKE_TEST_RU(test_requires_user2)
 * };
 * ```
 *
 * @param func The function to create a unit test out of.
 */
#define MAKE_TEST_RU(func) {func, #func, 1}

/**
 * @brief A package containing multiple unit tests.<br>
 * Do not initialize these directly. Use the MAKE_PKG() macro.
 *
 * @see MAKE_PKG()
 */
struct test_pkg{
	const struct unit_test* tests; /**< @brief The array of tests. */
	const size_t tests_len;        /**< @brief The number of tests in the array. */
	const char* name;              /**< @brief The name of the test package. */
};

/**
 * @brief Converts a unit test array into a unit test package.<br>
 * This must be called under the same scope as the test array in question.<br>
 * The package can be used like any other variable under the pkg_name specified.<br>
 * To export the package for use with another file, use the EXPORT_PKG() macro in a header file.
 * @see EXPORT_PKG()
 *
 * @param tests The array of tests to convert.<br>
 * This must be an array and not a pointer, otherwise the length determination will fail.
 *
 * @param pkg_name The name to create the package with.
 */
#define MAKE_PKG(tests, pkg_name) const struct test_pkg pkg_name = {tests, sizeof(tests) / sizeof(tests[0]), #tests}

/**
 * @brief Exports a package made with MAKE_PKG() for use with other files.<br>
 * This must be used in a header file seperate from the package definition.
 * @see MAKE_PKG()
 *
 * @param export_name The name of the package to export.<br>
 * This is the second parameter to the MAKE_PKG() macro.
 */
#define EXPORT_PKG(export_name) extern const struct test_pkg export_name

#define RT_NORMAL         (0x0) /**< @brief No special options. Only valid by itself. */
#define RT_NO_RU_TESTS    (0x1) /**< @brief Exclude all tests that require the user. */
#define RT_NO_NONRU_TESTS (0x2) /**< @brief Exclude all tests that don't require the user. */

/**
 * @brief Executes an array of packages.<br>
 * To run a single package, use run_single_pkg().
 * @see run_single_pkg()
 *
 * @param pkgs The array of packages to execute.
 *
 * @param pkgs_len The number of packages to execute.
 *
 * @param flags Special flags. These can be combined with the '|' operator.
 *
 * @return The number of tests that failed.
 */
int run_pkgs(const struct test_pkg** pkgs, size_t pkgs_len, unsigned flags);

/**
 * @brief Executes a single package.
 *
 * @param pkg The package to execute.
 *
 * @flags Special flags. These can be combined with the '|' operator.
 *
 * @return The number of tests that failed.
 */
int run_single_pkg(const struct test_pkg* pkg, unsigned flags);

#endif
