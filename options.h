/*
 * Command-line and menu-based option parsing module
 * Copyright (C) 2018 Jonathan Lemos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */



#ifndef __OPTIONS_H
#define __OPTIONS_H

/* compressor */
#include "maketar.h"

#define FLAG_VERBOSE (0x1)

typedef enum OPERATION{
	OP_INVALID = 0,
	OP_BACKUP = 1,
	OP_RESTORE = 2,
	OP_CONFIGURE = 3
}OPERATION;

typedef struct options{
	char*      prev_backup;
	char**     directories;
	int        directories_len;
	char**     exclude;
	int        exclude_len;
	char*      hash_algorithm;
	char*      enc_algorithm;
	COMPRESSOR comp_algorithm;
	char*      file_out;
	OPERATION  operation;
	unsigned   flags;
}options;

void usage(const char* progname);
int display_menu(const char** options, int num_options, const char* title);
int parse_options_cmdline(int argc, char** argv, options* out);
int parse_options_menu(options* opt);
int parse_options_fromfile(const char* file, options* opt);
int write_options_tofile(const char* file, options* opt);
void free_options(options* o);
int get_default_options(options* opt);

#endif
