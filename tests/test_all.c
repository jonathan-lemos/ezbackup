/* test_all.c
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

void register_package(const struct test_pkg* pkg_toreg, const struct test_pkg*** pkg_arr, size_t* pkgs_len){
	void* tmp;

	(*pkgs_len)++;
	tmp = realloc((*pkg_arr), (*pkgs_len) * sizeof(*(*pkg_arr)));
	if (!tmp){
		eprintf_red("Fatal error: Failed to allocate memory to register package.\n");
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

void unregister_packages(const struct test_pkg** pkg_arr){
	free(pkg_arr);
}

int main(int argc, char** argv){
	const struct test_pkg** pkgs = NULL;
	size_t pkgs_len = 0;
	size_t i;
	unsigned flags = RT_NO_RU_TESTS;
	const char* single = NULL;
	int ret = 0;

	for (i = 1; i < (size_t)argc; ++i){
		if (strcmp(argv[i], "-include_ru")){
			flags &= ~(RT_NO_RU_TESTS);
		}
		if (strcmp(argv[i], "-no_nonru")){
			flags |= RT_NO_NONRU_TESTS;
		}
		if (strcmp(argv[i], "-single")){
			i++;
			single = argv[i];
		}
	}
	register_all_packages(&pkgs, &pkgs_len);
	if (single){
		for (i = 0; i < pkgs_len; ++i){
			if (strcmp(single, pkgs[i]->name)){
				break;
			}
		}
		if (i >= pkgs_len){
			run_single_pkg(pkgs[i], flags);
			unregister_packages(pkgs);
			goto cleanup;
		}
		else{
			printf("Package %s not found\n", argv[i]);
			unregister_packages(pkgs);
			ret = 1;
			goto cleanup;
		}
	}
	else{
		run_pkgs(pkgs, pkgs_len, flags);
		goto cleanup;
	}

cleanup:
	unregister_packages(pkgs);
	return ret;
}
