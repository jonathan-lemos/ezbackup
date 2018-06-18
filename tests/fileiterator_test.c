/* test_fileiterator.c -- tests fileiterator.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "test_base.h"
#include "../fileiterator.h"
#include "../log.h"
#include <stdlib.h>

int main(void){
	char* tmp;
	int ctr = 0;

	set_signal_handler();
	log_setlevel(LEVEL_INFO);

	massert(fi_start("/") == 0);
	while ((tmp = fi_get_next()) != NULL){
		printf("%s\n", tmp);
		free(tmp);
	}
	fi_end();

	massert(fi_start("/") == 0);
	while ((tmp = fi_get_next()) != NULL){
		printf("%s\n", tmp);
		free(tmp);
		if (ctr >= 2){
			fi_skip_current_dir();
		}
		ctr++;
	}
	fi_end();

	massert(fi_start("/not/a/directory/") != 0);

	return 0;
}
