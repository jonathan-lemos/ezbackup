/* main.c -- contains main function
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "log.h"
#include "options/options.h"
#include "backup.h"

int main(int argc, char** argv){
	struct options* opt;
	enum OPERATION op;

	log_setlevel(LEVEL_INFO);

	if (argc > 1){
		int res;
		if ((res = parse_options_cmdline(argc, argv, &opt, &op)) != 0){
			fprintf(stderr, "Argument %s is invalid\n", res >= 0 ? argv[res] : "NULL");
			return -1;
		}
	}
	else{
		if ((op = parse_options_menu(&opt)) == OP_INVALID){
			log_error("Failed to parse options from menu");
			return -1;
		}
	}

	do{
		switch (op){
		case OP_BACKUP:
			if (backup(opt) != 0){
				log_error("Backup failed");
			}
			break;
		case OP_RESTORE:
			fprintf(stderr, "Restore not implemented yet\n");
			break;
		case OP_CONFIGURE:
			op = parse_options_menu(&opt);
			break;
		case OP_EXIT:
			return 0;
			break;
		case OP_INVALID:
			log_error("Invalid operation chosen");
			return -1;
		}
	}while (op == OP_CONFIGURE);

	return 0;
}
