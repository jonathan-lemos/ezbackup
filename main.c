/* main.c -- contains main function
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* iterates files */
#include "fileiterator.h"
/* makes the tar */
#include "maketar.h"
/* read_file() */
#include "readfile.h"
/* error handling */
#include "error.h"
#include <errno.h>
/* making hashes */
#include "checksum.h"
/* encryption */
#include "crypt.h"
/* verbose output */
#include "progressbar.h"
/* read files */
#include "readfile.h"
/* command line args */
#include "options.h"
/* mega integration */
#include "cloud/mega.h"

#include "backup.h"
/* printf, FILE* */
#include <stdio.h>
/* strerror(), strcmp(), etc. */
#include <string.h>
/* malloc() */
#include <stdlib.h>
/* stat to check if directory */
#include <sys/stat.h>
/* getting home directory */
#include <unistd.h>
#include <pwd.h>
/* strftime */
#include <time.h>
/* disabling core dumps */
#include <sys/resource.h>
/* readline) */
#if defined(__linux__)
#include <editline/readline.h>
#elif defined(__APPLE__)
#include <readline/readline.h>
#else
#error "This operating system is not supported"
#endif

#define UNUSED(x) ((void)(x))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

static int parse_options_prev(int argc, char** argv, struct options* opt){
	int res;
	/* if we have command line args */
	if (argc >= 2){
		int parse_res;

		parse_res = parse_options_cmdline(argc, argv, opt);
		if (parse_res < 0 || parse_res > argc){
			log_error(__FL__, "Failed to parse command line arguments");
			return -1;
		}
		else if (parse_res != 0){
			printf("Invalid parameter %s\n", argv[parse_res]);
			return -1;
		}
		if (opt->operation == OP_INVALID){
			printf("No operation specified. Specify one of \"backup\", \"restore\", \"configure\".\n");
			return -1;
		}
		return 0;
	}

	res = read_config_file(opt);
	if (res < 0){
		log_error(__FL__, "Failed to parse options from file");
		return -1;
	}
	else if (res > 0){
		log_info(__FL__, "Options file does not exist");

		get_default_options(opt);
		return 1;
	}
	return 0;
}

static int parse_options_current(const options* prev, options* out, const char* title){
	int res;
	const char* choices_initial[] = {
		"Backup",
		"Restore",
		"Configure",
		"Exit"
	};
	memcpy(out, prev, sizeof(*out));

	/* else get options from menu or config file */
	do{
		res = display_menu(choices_initial, ARRAY_SIZE(choices_initial), title ? title : "Main Menu");
		switch (res){
		case 0:
			out->operation = OP_BACKUP;
			break;
		case 1:
			out->operation = OP_RESTORE;
			break;
		case 2:
			out->operation = OP_CONFIGURE;
			parse_options_menu(out);
			break;
		case 3:
			out->operation = OP_EXIT;
			return 0;
		default:
			log_fatal(__FL__, "Option %d chosen of %d. This should never happen", res + 1, ARRAY_SIZE(choices_initial));
			return -1;
		}
	}while (out->operation == OP_CONFIGURE);

	return 0;
}

static int restore(func_params* fparams, const options* opt_prev){
	int res;
	const char* options_main[] = {
		"Restore locally",
		"Restore from cloud"
	};

	res = display_menu(options_main, ARRAY_SIZE(options_main), "Restore");
	switch (res){
	case 0:
		;
	case 1:

	}
}

#ifndef __UNIT_TESTING__
int main(int argc, char** argv){
	func_params fparams;
	options opt_prev;
	int res;

	/* set fparams values to all NULL or 0 */
	fparams.tp = NULL;
	fparams.fp_hashes = NULL;

	log_setlevel(LEVEL_INFO);

	/* parse command line args */
	res = parse_options_prev(argc, argv, &opt_prev);
	if (res < 0){
		log_debug(__FL__, "Error parsing options");
		return 1;
	}
	else if (res > 0){
		parse_options_current(&opt_prev, &(fparams.opt), "Main Menu - Default Options Generated");
	}
	else{
		if (write_config_file(&fparams) != 0){
			log_warning(__FL__, "Could not write config file");
		}
		parse_options_current(&opt_prev, &(fparams.opt), NULL);
	}

	switch (fparams.opt.operation){
	case OP_BACKUP:
		backup(&fparams.opt, &opt_prev);
		break;
	case OP_RESTORE:
		printf("Restore not implemented yet\n");
		return 0;
	case OP_EXIT:
		return 0;
	default:
		log_fatal(__FL__, "Invalid operation specified. This should never happen");
		return 1;
	}

	return 0;
}
#endif
