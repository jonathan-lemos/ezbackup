/** @file main.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "log.h"
#include "options/options.h"
#include "options/options_menu.h"
#include "backup.h"

int main(int argc, char** argv){
	struct options* opt = NULL;
	enum OPERATION op;
	int ret = 0;

	log_setlevel(LEVEL_WARNING);

	if (argc > 1){
		int res;
		if ((res = parse_options_cmdline(argc, argv, &opt, &op)) != 0){
			fprintf(stderr, "Argument %s is invalid\n", res >= 0 ? argv[res] : "NULL");
			ret = 1;
			goto cleanup;
		}
	}
	else{
		if (get_prev_options(&opt) < 0){
			log_error("Failed to read previous options");
			ret = 1;
			goto cleanup;
		}
		while ((op = menu_operation()) == OP_CONFIGURE){
			menu_configure(opt);
			set_prev_options(opt);
		}
	}

	switch (op){
	case OP_BACKUP:
		if (backup(opt) != 0){
			log_error("Backup failed");
		}
		break;
	case OP_RESTORE:
		fprintf(stderr, "Restore not implemented yet\n");
		ret = 1;
		goto cleanup;
		break;
	case OP_EXIT:
		ret = 0;
		goto cleanup;
		break;
	default:
		log_error("Invalid operation chosen");
		ret = 1;
		goto cleanup;
	}

cleanup:
	options_free(opt);
	return ret;
}
