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
#include "../compression/zip.h"
#include "../cloud/include.h"
#include "../strings/stringarray.h"
#include <openssl/evp.h>

enum OPERATION{
	OP_INVALID = -1,
	OP_BACKUP  = 1,
	OP_RESTORE = 2,
	OP_CONFIGURE = 3,
	OP_EXIT = 4
};

struct options{
	char*                 prev_backup;
	struct string_array*  directories;
	struct string_array*  exclude;
	const EVP_MD*         hash_algorithm;
	const EVP_CIPHER*     enc_algorithm;
	char*                 enc_password;
	enum COMPRESSOR       comp_algorithm;
	int                   comp_level;
	char*                 output_directory;
	struct cloud_options* cloud_options;
	union tagflags{
		struct tagbits{
			unsigned      flag_verbose: 1;
		}bits;
		unsigned          dword;
	}flags;
};

void version(void);
void usage(const char* progname);

struct options* options_new(void);
int options_cmp(const struct options* opt1, const struct options* opt2);
void options_free(struct options* o);

enum OPERATION parse_options_menu(struct options** opt);
int parse_options_cmdline(int argc, char** argv, struct options** out, enum OPERATION* op_out);
int parse_options_fromfile(const char* file, struct options** output);
int write_options_tofile(const char* file, const struct options* opt);

int set_last_backup_dir(const char* dir);
int get_last_backup_dir(char** out);

#ifdef __UNIT_TESTING__
int get_last_backup_file(char** out);
#endif

#endif
