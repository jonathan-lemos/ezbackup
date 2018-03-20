#ifndef __OPTIONS_H
#define __OPTIONS_H

/* compressor */
#include "maketar.h"

#define FLAG_VERBOSE (0x1)

typedef struct options {
	char*      prev_backup;
	char**     directories;
	int        directories_len;
	char**     exclude;
	int        exclude_len;
	char*      hash_algorithm;
	char*      enc_algorithm;
	COMPRESSOR comp_algorithm;
	char*      file_out;
	unsigned   flags;
}options;

void usage(const char* progname);
int display_menu(const char** options, int num_options, const char* title);
int parse_options_cmdline(int argc, char** argv, options* out);
int parse_options_menu(options* opt);
int parse_options_fromfile(const char* file, options* opt);
int write_options_tofile(const char* file, options* opt);
void free_options(options* o);

#endif
