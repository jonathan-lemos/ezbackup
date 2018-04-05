/* options.h -- command-line/menu-based options parser
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __OPTIONS_H
#define __OPTIONS_H

/* compressor */
#include "maketar.h"
#include <openssl/evp.h>

#define FLAG_VERBOSE (0x1)

typedef enum OPERATION{
	OP_INVALID = 0,
	OP_BACKUP  = 1,
	OP_RESTORE = 2,
	OP_CONFIGURE = 3,
	OP_EXIT = 4
}OPERATION;

struct options{
	char*             prev_backup;
	char**            directories;
	int               directories_len;
	char**            exclude;
	int               exclude_len;
	const EVP_MD*     hash_algorithm;
	const EVP_CIPHER* enc_algorithm;
	COMPRESSOR        comp_algorithm;
	int               comp_level;
	char*             output_directory;
	OPERATION         operation;
	unsigned          flags;
};

void version(void);
void usage(const char* progname);
int display_menu(const char** options, int num_options, const char* title);
int parse_options_cmdline(int argc, char** argv, struct options* out);
int parse_options_menu(struct options* opt);
int parse_options_fromfile(const char* file, struct options* opt);
int write_options_tofile(const char* file, struct options* opt);
void free_options(struct options* o);
int get_default_options(struct options* opt);

#endif
