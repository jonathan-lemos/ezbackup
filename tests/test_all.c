/** @file tests/test_all.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "test_framework.h"
#include "backup_test.h"
#include "checksum_test.h"
#include "cli_test.h"
#include "coredumps_test.h"
#include "filehelper_test.h"
#include "fileiterator_test.h"
#include "log_test.h"
#include "progressbar_test.h"
#include "cloud/base_test.h"
#include "cloud/cloud_options_test.h"
#include "compression/zip_test.h"
#include "crypt/crypt_test.h"
#include "crypt/crypt_easy_test.h"
#include "crypt/crypt_getpassword_test.h"
#include "options/options_test.h"
#include "options/options_file_test.h"
#include "options/options_menu_test.h"
#include "strings/stringarray_test.h"
#include "strings/stringhelper_test.h"
#include <stdlib.h>
#include <string.h>
#include "../cli.h"

void register_package(const struct test_pkg* pkg_toreg, const struct test_pkg*** pkg_arr, size_t* pkgs_len){
	void* tmp;

	(*pkgs_len)++;
	tmp = realloc((*pkg_arr), (*pkgs_len) * sizeof(*(*pkg_arr)));
	if (!tmp){
		eprintf_red("Internal Error: Failed to allocate memory to register package.\n");
		exit(1);
	}
	(*pkg_arr) = tmp;
	(*pkg_arr)[(*pkgs_len) - 1] = pkg_toreg;
}

void register_all_packages(const struct test_pkg*** pkg_arr, size_t* pkgs_len){
	register_package(&backup_pkg, pkg_arr, pkgs_len);
	register_package(&checksum_pkg, pkg_arr, pkgs_len);
	register_package(&cli_pkg, pkg_arr, pkgs_len);
	register_package(&coredumps_pkg, pkg_arr, pkgs_len);
	register_package(&filehelper_pkg, pkg_arr, pkgs_len);
	register_package(&fileiterator_pkg, pkg_arr, pkgs_len);
	register_package(&log_pkg, pkg_arr, pkgs_len);
	register_package(&progressbar_pkg, pkg_arr, pkgs_len);
	register_package(&cloud_base_pkg, pkg_arr, pkgs_len);
	register_package(&cloud_options_pkg, pkg_arr, pkgs_len);
	register_package(&compression_zip_pkg, pkg_arr, pkgs_len);
	register_package(&crypt_pkg, pkg_arr, pkgs_len);
	register_package(&crypt_easy_pkg, pkg_arr, pkgs_len);
	register_package(&crypt_getpassword_pkg, pkg_arr, pkgs_len);
	register_package(&options_pkg, pkg_arr, pkgs_len);
	register_package(&options_file_pkg, pkg_arr, pkgs_len);
	register_package(&options_menu_pkg, pkg_arr, pkgs_len);
	register_package(&stringarray_pkg, pkg_arr, pkgs_len);
	register_package(&stringhelper_pkg, pkg_arr, pkgs_len);
}

void add_string_toarr(const char* str, const char*** str_arr, size_t* arr_len){
	void* tmp;

	(*arr_len)++;
	tmp = realloc((*str_arr), (*arr_len) * sizeof(**str_arr));
	if (!tmp){
		eprintf_red("Internal Error: Failed to allocate requested memory for string array.");
		exit(1);
	}
	*str_arr = tmp;
	(*str_arr)[(*arr_len) - 1] = str;
}

void display_mm(const struct test_pkg** pkgs, size_t pkgs_len, const struct test_pkg*** out, size_t* out_len, unsigned* out_flags){
	const char** str_arr = NULL;
	size_t arr_len = 0;
	size_t i;
	int res;

	add_string_toarr("All Tests", &str_arr, &arr_len);
	add_string_toarr("All Tests (Including RU)", &str_arr, &arr_len);
	add_string_toarr("Only RU tests", &str_arr, &arr_len);
	for (i = 0; i < pkgs_len; ++i){
		add_string_toarr(pkgs[i]->name, &str_arr, &arr_len);
	}

	res = display_menu(str_arr, arr_len, "Select a test package");
	if (res < 0){
		eprintf_red("Internal Error: Invalid menu entry chosen.");
		exit(1);
	}
	switch (res){
	case 0:
		*out = pkgs;
		*out_len = pkgs_len;
		*out_flags = RT_NO_RU_TESTS;
		break;
	case 1:
		*out = pkgs;
		*out_len = pkgs_len;
		*out_flags = RT_NORMAL;
		break;
	case 2:
		*out = pkgs;
		*out_len = pkgs_len;
		*out_flags = RT_NO_NONRU_TESTS;
		break;
	default:
		*out = pkgs + (res - 3);
		*out_len = 1;
		*out_flags = RT_NORMAL;
	}
	free(str_arr);
}

int main(void){
	const struct test_pkg** pkgs = NULL;
	size_t pkgs_len = 0;
	const struct test_pkg** pkgs_run = NULL;
	size_t pkgs_run_len = 0;
	unsigned flags = 0;
	int ret = 0;

	register_all_packages(&pkgs, &pkgs_len);
	display_mm(pkgs, pkgs_len, &pkgs_run, &pkgs_run_len, &flags);
	run_pkgs(pkgs_run, pkgs_run_len, flags);

	free(pkgs);
	return ret;
}
