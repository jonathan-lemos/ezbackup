/* test_fileiterator.c -- tests fileiterator.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "test_base.h"
#include "../fileiterator.h"
#include "string.h"

static int func(const char* file, const char* dir, struct stat* st, void* params){
	(void)dir;
	(void)st;
	(void)params;
	printf("%s\n", file);
	return 1;
}

static int err(const char* file, int __errno, void* params){
	(void)params;
	printf("Error opening %s (%s)\n", file, strerror(__errno));
	return 1;
}

int main(void){
	set_signal_handler();

	enum_files("/", func, NULL, err, NULL);
	return 0;
}
