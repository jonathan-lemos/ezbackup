/* options.c -- command-line/menu-based options parser
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* prototypes */
#include "options.h"

#include "options_menu.h"

#include "options_file.h"
/* errors */
#include "../log.h"
#include <errno.h>

#include "../crypt/base16.h"

#include "../filehelper.h"

#include "../strings/stringhelper.h"
/* printf */
#include <stdio.h>
/* strcmp */
#include <string.h>
/* malloc */
#include <stdlib.h>

/* char* to EVP_MD(*)(void) */
#include <openssl/evp.h>
/* command-line tab completion */
#include "../readline_include.h"
/* is_directory() */
#include <sys/stat.h>
/* get home directory */
#include <unistd.h>
#include <pwd.h>
#include <termios.h>

#ifndef PROG_NAME
#define PROG_NAME NULL
#endif
#ifndef PROG_VERSION
#define PROG_VERSION NULL
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void version(void){
	const char* program_name = PROG_NAME;
	const char* version      = PROG_VERSION;
	const char* year         = "2018";
	const char* name         = "Jonathan Lemos";
	const char* license      = "This software may be modified and distributed under the terms of the MIT license.";

	if (!program_name){
		log_fatal("PROG_NAME not specified");
		exit(1);
	}

	if (!version){
		log_fatal("PROG_VERSION not specified");
		exit(1);
	}

	printf("%s %s\n", program_name, version);
	printf("Copyright (c) %s %s\n", year, name);
	printf("%s\n", license);
}

void usage(const char* progname){
	return_ifnull(progname, ;);

	printf("Usage: %s (backup|restore|configure) [options]\n", progname);
	printf("Options:\n");
	printf("\t-c, --compressor <gz|bz2|...>\n");
	printf("\t-C, --checksum <md5|sha1|...>\n");
	printf("\t-d, --directories </dir1 /dir2 /...>\n");
	printf("\t-e, --encryption <aes-256-cbc|seed-ctr|...>\n");
	printf("\t-h, --help\n");
	printf("\t-i, --cloud <mega|...>\n");
	printf("\t-I, --upload_directory </dir1/dir2/...>\n");
	printf("\t-o, --output </out/dir>\n");
	printf("\t-p, --password <password>\n");
	printf("\t-q, --quiet\n");
	printf("\t-u, --username <username>");
	printf("\t-x, --exclude </dir1 /dir2 /...>\n");
}

static int get_backup_directory(char** out){
	struct passwd* pw;
	const char* homedir;
	struct stat st;

	return_ifnull(out, -1);

	/* get home directory */
	if (!(homedir = getenv("HOME"))){
		pw = getpwuid(getuid());
		if (!pw){
			log_error("Failed to get home directory");
			return -1;
		}
		homedir = pw->pw_dir;
	}

	/* /home/<user>/Backups */
	*out = malloc(strlen(homedir) + sizeof("/Backups"));
	if (!(*out)){
		log_enomem();
		return -1;
	}
	strcpy(*out, homedir);
	strcat(*out, "/Backups");

	if (stat(*out, &st) == -1){
		if (mkdir(*out, 0755) == -1){
			log_error_ex("Failed to create backup directory at %s", *out);
			free(*out);
			return -1;
		}
	}

	return 0;
}

/* parses command line args
 *
 * returns -1 if out is NULL, 0 on success
 * index of bad argument on bad argument */
int parse_options_cmdline(int argc, char** argv, struct options** output, enum OPERATION* out_op){
	int i;
	struct options* out = *output;

	if ((out = options_new()) != 0){
		log_debug("Failed to get default options");
		return -1;
	}

	*out_op = OP_INVALID;

	for (i = 1; i < argc; ++i){
		if (!strcmp(argv[i], "--version")){
			version();
			exit(0);
		}
		if (!strcmp(argv[i], "-h") ||
				!strcmp(argv[i], "--help")){
			usage(argv[0]);
			exit(0);
		}
		/* compression */
		else if (!strcmp(argv[i], "-c") ||
				!strcmp(argv[i], "--compressor")){
			/* check next argument */
			++i;
			out->comp_algorithm = get_compressor_byname(argv[i]);
		}
		/* checksum */
		else if (!strcmp(argv[i], "-C") ||
				!strcmp(argv[i], "--checksum")){
			/* check next argument */
			++i;
			OpenSSL_add_all_algorithms();
			out->hash_algorithm = EVP_get_digestbyname(argv[i]);
		}
		/* encryption */
		else if (!strcmp(argv[i], "-e") ||
				!strcmp(argv[i], "--encryption")){
			/* next argument */
			++i;
			OpenSSL_add_all_algorithms();
			out->enc_algorithm = EVP_get_cipherbyname(argv[i]);
		}
		/* verbose */
		else if (!strcmp(argv[i], "-q") ||
				!strcmp(argv[i], "--quiet")){
			out->flags.bits.flag_verbose = 0;
		}
		/* outfile */
		else if (!strcmp(argv[i], "-o") ||
				!strcmp(argv[i], "--output")){
			/* next argument */
			++i;

			if (out->output_directory){
				free(out->output_directory);
			}
			/* must be able to call free() w/o errors */
			out->output_directory = malloc(strlen(argv[i]) + 1);
			strcpy(out->output_directory, argv[i]);
		}
		/* exclude */
		else if (!strcmp(argv[i], "-x") ||
				!strcmp(argv[i], "--exclude")){
			while (++i < argc && argv[i][0] != '-'){
				sa_add(out->exclude, argv[i]);
			}
			--i;
		}
		/* directories */
		else if (!strcmp(argv[i], "-d") ||
				!strcmp(argv[i], "--directories")){
			while (++i < argc && argv[i][0] != '-'){
				sa_add(out->directories, argv[i]);
			}
			--i;
		}
		/* username */
		else if (!strcmp(argv[i], "-u") ||
				!strcmp(argv[i], "--username")){
			++i;
			if (co_set_username(out->cloud_options, argv[i]) != 0){
				log_debug("Failed to set cloud_options username");
				return -1;
			}
		}
		/* password */
		else if (!strcmp(argv[i], "-p") ||
				!strcmp(argv[i], "--password")){
			++i;
			if (co_set_password(out->cloud_options, argv[i]) != 0){
				log_debug("Failed to set cloud_options password");
				return -1;
			}
		}
		else if (!strcmp(argv[i], "-i") ||
				!strcmp(argv[i], "--cloud")){
			++i;
			out->cloud_options->cp = cloud_provider_from_string(argv[i]);
		}
		else if (!strcmp(argv[i], "-I") ||
				!strcmp(argv[i], "--upload_directory")){
			++i;
			if (co_set_upload_directory(out->cloud_options, argv[i]) != 0){
				log_debug("Failed to set cloud_options upload directory");
				return -1;
			}
		}
		/* operation */
		else if (argv[i][0] != '-'){
			if (!strcmp(argv[i], "backup")){
				*out_op = OP_BACKUP;
			}
			else if (!strcmp(argv[i], "restore")){
				*out_op = OP_RESTORE;
			}
			else if (!strcmp(argv[i], "configure")){
				*out_op = OP_CONFIGURE;
			}
			else{
				return i;
			}
		}
		else{
			return i;
		}
	}

	if (out->directories->len == 0){
		sa_add(out->directories, "/");
	}
	if (!out->output_directory && get_backup_directory(&out->output_directory) != 0){
		log_error("Could not determine output directory");
		return -1;
	}
	return 0;
}

struct options* options_new(void){
	struct options* opt = malloc(sizeof(*opt));
	if (!opt){
		log_enomem();
		return NULL;
	}
	opt->prev_backup = NULL;
	opt->directories = sa_new();
	opt->exclude = sa_new();
	opt->hash_algorithm = EVP_sha1();
	opt->enc_algorithm = EVP_aes_256_cbc();
	opt->enc_password = NULL;
	opt->comp_algorithm = COMPRESSOR_GZIP;
	opt->comp_level = 0;
	if (get_backup_directory(&(opt->output_directory)) != 0){
		log_debug("Failed to make backup directory");
		return NULL;
	}
	opt->cloud_options = co_new();
	opt->flags.dword = 0;
	opt->flags.bits.flag_verbose = 1;

	return opt;
}

/*
   static char* read_file_string(FILE* in){
   int c;
   char* ret = NULL;
   int ret_len = 0;

   while ((c = fgetc(in)) != '\0'){
   if (c == EOF){
   log_debug("Reached EOF");
   free(ret);
   return NULL;
   }
   ret_len++;
   ret = realloc(ret, ret_len);
   if (!ret){
   log_enomem();
   return NULL;
   }
   ret[ret_len - 1] = c;
   }
   ret_len++;
   ret = realloc(ret, ret_len);
   ret[ret_len - 1] = '\0';

   return ret;
   }
   */

int parse_options_fromfile(const char* file, struct options** output){
	struct options* opt = NULL;
	struct opt_entry** entries = NULL;
	size_t entries_len = 0;
	int ret = 0;
	int res = 0;

	*output = options_new();
	if (!(*output)){
		log_enomem();
		ret = -1;
		goto cleanup;
	}

	opt = *output;

	if (read_option_file(file, &entries, &entries_len) != 0){
		log_error("Failed to read options file");
		ret = -1;
		goto cleanup;
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "PREV_BACKUP");
	if (res >= 0){
		opt->prev_backup = entries[res]->value;
	}
	else{
		log_warning("Key PREV_BACKUP missing from file");
		opt->prev_backup = NULL;
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "DIRECTORIES");
	if (res >= 0){
		const char* str = entries[res]->value;
		size_t ptr;
		for (ptr = 0; ptr < entries[res]->value_len; ptr += strlen(&(str[ptr])) + 1){
			if (sa_add(opt->directories, &(str[ptr])) != 0){
				log_warning("Failed to add string to directories array");
			}
		}
	}
	else{
		log_warning("Key DIRECTORIES missing from file");
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "EXCLUDE");
	if (res >= 0){
		const char* str = entries[res]->value;
		size_t ptr;
		for (ptr = 0; ptr < entries[res]->value_len; ptr += strlen(str) + 1){
			if (sa_add(opt->exclude, &(str[ptr])) != 0){
				log_warning("Failed to add string to exclude array");
			}
		}
	}
	else{
		log_warning("Key EXCLUDE missing from file");
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "HASH_ALGORITHM");
	if (res >= 0){
		opt->hash_algorithm = EVP_get_digestbyname(entries[res]->value);
	}
	else{
		log_warning("Key HASH_ALGORITHM missing from file");
		opt->hash_algorithm =  NULL;
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "ENC_ALGORITHM");
	if (res >= 0){
		opt->enc_algorithm = EVP_get_cipherbyname(entries[res]->value);
	}
	else{
		log_warning("Key ENC_ALGORITHM missing from file");
		opt->hash_algorithm =  NULL;
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "ENC_PASSWORD");
	if (res >= 0){
		if (entries[res]->value && from_base16(entries[res]->value, (void**)&opt->enc_password, NULL) != 0){
			log_warning("Failed to read ENC_PASSWORD");
		}
		else if (!entries[res]->value){
			opt->enc_password = NULL;
		}
	}
	else{
		log_warning("Key ENC_PASSWORD missing from file");
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "COMP_ALGORITHM");
	if (res >= 0){
		opt->comp_algorithm = *(int*)entries[res]->value - '0';
	}
	else{
		log_warning("Key COMP_ALGORITHM missing from file");
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "COMP_LEVEL");
	if (res >= 0){
		opt->comp_level = *(int*)entries[res]->value - '0';
	}
	else{
		log_warning("Key COMP_LEVEL missing from file");
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "OUTPUT_DIRECTORY");
	if (res >= 0){
		free(opt->output_directory);
		opt->output_directory = sh_dup(entries[res]->value);
		if (!opt->output_directory){
			log_warning("Failed to read output_directory for file");
		}
	}
	else{
		log_warning("Key OUTPUT_DIRECTORY missing from file");
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "CO_CP");
	if (res >= 0){
		opt->cloud_options->cp = *(int*)entries[res]->value - '0';
	}
	else{
		log_warning("Key CO_CP missing from file");
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "CO_USERNAME");
	if (res >= 0){
		free(opt->cloud_options->username);
		opt->cloud_options->username = sh_dup(entries[res]->value);
		if (!opt->cloud_options->username){
			log_warning("Failed to read username from file");
		}
	}
	else{
		log_warning("Key CO_USERNAME missing from file");
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "CO_PASSWORD");
	if (res >= 0){
		free(opt->cloud_options->password);
		if (entries[res]->value && from_base16(entries[res]->value, (void**)&opt->cloud_options->password, NULL) != 0){
			log_warning("Failed to read CO_PASSWORD");
		}
		else if (!entries[res]->value){
			opt->cloud_options->password = NULL;
		}
	}
	else{
		log_warning("Key CO_PASSWORD missing from file");
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "CO_UPLOAD_DIRECTORY");
	if (res >= 0){
		free(opt->cloud_options->upload_directory);
		opt->cloud_options->upload_directory = sh_dup(entries[res]->value);
		if (!opt->cloud_options->upload_directory){
			log_warning("Failed to read password from file");
		}
	}
	else{
		log_warning("Key CO_UPLOAD_DIRECTORY missing from file");
	}

	res = binsearch_opt_entries((const struct opt_entry* const*)entries, entries_len, "FLAGS");
	if (res >= 0){
		opt->flags.dword = *(unsigned*)entries[res]->value;
	}
	else{
		log_warning("Key FLAGS missing from file");
		opt->flags.dword = 0;
		opt->flags.bits.flag_verbose = 1;
	}

cleanup:
	if (ret != 0){
		free(opt);
	}
	free_opt_entry_array(entries, entries_len);
	return ret;
}

int write_options_tofile(const char* file, const struct options* opt){
	FILE* fp = NULL;
	unsigned char* tmp = NULL;
	unsigned char* tmp_old = NULL;
	char* tmp_pw = NULL;
	size_t tmp_len = 0;
	int tmp_int = 0;
	size_t i;
	int ret = 0;

	fp = create_option_file(file);
	if (!fp){
		log_error("Failed to create option file");
		ret = -1;
		goto cleanup;
	}

	if (add_option_tofile(fp, "PREV_BACKUP", (const unsigned char*)opt->prev_backup, opt->prev_backup ? strlen(opt->prev_backup) + 1 : 0) != 0){
		log_warning("Failed to add PREV_BACKUP to file");
	}

	/* get length of all strings including their '\0''s */
	tmp_len = 0;
	for (i = 0; i < opt->directories->len; ++i){
		tmp_len += strlen(opt->directories->strings[i]) + 1;
	}
	/* allocate buffer */
	tmp = malloc(tmp_len);
	if (!tmp){
		log_enomem();
		return -1;
	}
	/* save beginning of buffer */
	tmp_old = tmp;
	for (i = 0; i < opt->directories->len; ++i){
		/* copy the string and its '\0' over to tmp */
		memcpy(tmp, opt->directories->strings[i], strlen(opt->directories->strings[i]) + 1);
		/* move tmp past the string we copied */
		tmp += strlen(opt->directories->strings[i]) + 1;
	}
	/* restore beginning of buffer */
	tmp = tmp_old;
	tmp_old = NULL;
	if (add_option_tofile(fp, "DIRECTORIES", tmp, tmp_len) != 0){
		log_warning("Failed to add DIRECTORIES to file");
	}
	free(tmp);
	tmp = NULL;

	tmp_len = 0;
	for (i = 0; i < opt->exclude->len; ++i){
		tmp_len += strlen(opt->exclude->strings[i]) + 1;
	}
	tmp = malloc(tmp_len);
	if (!tmp){
		log_enomem();
		return -1;
	}
	tmp_old = tmp;
	for (i = 0; i < opt->exclude->len; ++i){
		memcpy(tmp, opt->exclude->strings[i], strlen(opt->exclude->strings[i]) + 1);
		tmp += strlen(opt->exclude->strings[i]) + 1;
	}
	tmp = tmp_old;
	tmp_old = NULL;
	if (add_option_tofile(fp, "EXCLUDE", tmp, tmp_len) != 0){
		log_warning("Failed to add EXCLUDE to file");
	}
	free(tmp);
	tmp = NULL;

	if (add_option_tofile(fp, "HASH_ALGORITHM", EVP_MD_name(opt->hash_algorithm), strlen(EVP_MD_name(opt->hash_algorithm)) + 1) != 0){
		log_warning("Failed to add HASH_ALGORITHM to file");
	}

	if (add_option_tofile(fp, "ENC_ALGORITHM", EVP_CIPHER_name(opt->enc_algorithm), strlen(EVP_CIPHER_name(opt->enc_algorithm)) + 1) != 0){
		log_warning("Failed to add ENC_ALGORITHM to file");
	}

	if (opt->enc_password && to_base16(opt->enc_password, strlen(opt->enc_password) + 1, &tmp_pw) != 0){
		log_warning("Failed to convert pw to base16");
	}
	if (tmp_pw && add_option_tofile(fp, "ENC_PASSWORD", tmp_pw, strlen(tmp_pw) + 1) != 0){
		log_warning("Failed to add ENC_PASSWORD to file");
	}
	/* !tmp_pw needed otherwise this fires when tmp_pw is true and add_option_tofile == 0 */
	else if (!tmp_pw && add_option_tofile(fp, "ENC_PASSWORD", NULL, 0) != 0){
		log_warning("Failed to add ENC_PASSWORD to file");
	}
	free(tmp_pw);
	tmp_pw = NULL;

	tmp_int = opt->comp_algorithm + '0';
	if (add_option_tofile(fp, "COMP_ALGORITHM", &tmp_int, sizeof(tmp_int)) != 0){
		log_warning("Failed to add COMP_ALGORITHM to file");
	}

	tmp_int = opt->comp_level + '0';
	if (add_option_tofile(fp, "COMP_LEVEL", &tmp_int, sizeof(tmp_int)) != 0){
		log_warning("Failed to add COMP_LEVEL to file");
	}

	if (add_option_tofile(fp, "OUTPUT_DIRECTORY", opt->output_directory, strlen(opt->output_directory) + 1) != 0){
		log_warning("Failed to add OUTPUT_DIRECTORY to file");
	}

	tmp_int = opt->cloud_options->cp + '0';
	if (add_option_tofile(fp, "CO_CP", &tmp_int, sizeof(tmp_int)) != 0){
		log_warning("Failed to add CO_CP to file");
	}

	if (add_option_tofile(fp, "CO_USERNAME", opt->cloud_options->username, opt->cloud_options->username ? strlen(opt->cloud_options->username) + 1 : 0) != 0){
		log_warning("Failed to add CO_USERNAME to file");
	}

	if (opt->cloud_options->password && to_base16(opt->cloud_options->password, strlen(opt->cloud_options->password) + 1, &tmp_pw) != 0){
		log_warning("Failed to convert cloud password to base16");
	}
	if (tmp_pw && add_option_tofile(fp, "CO_PASSWORD", tmp_pw, strlen(tmp_pw) + 1) != 0){
		log_warning("Failed to add CO_PASSWORD to file");
	}
	else if (add_option_tofile(fp, "CO_PASSWORD", NULL, 0) != 0){
		log_warning("Failed to add CO_PASSWORD to file");
	}
	free(tmp_pw);
	tmp_pw = NULL;

	if (add_option_tofile(fp, "CO_UPLOAD_DIRECTORY", opt->cloud_options->upload_directory, strlen(opt->cloud_options->upload_directory) + 1) != 0){
		log_warning("Failed to add CO_UPLOAD_DIRECTORY to file");
	}

	if (add_option_tofile(fp, "FLAGS", &opt->flags.dword, sizeof(opt->flags.dword)) != 0){
		log_warning("Failed to add FLAGS to file");
	}

cleanup:
	fp ? fclose(fp) : 0;
	free(tmp);
	free(tmp_old);
	free(tmp_pw);
	return ret;
}

void options_free(struct options* opt){
	free(opt->prev_backup);
	sa_free(opt->directories);
	sa_free(opt->exclude);
	free(opt->enc_password);
	free(opt->output_directory);
	co_free(opt->cloud_options);
	free(opt);
}

int options_cmp(const struct options* opt1, const struct options* opt2){
	if (sh_cmp_nullsafe(opt1->prev_backup, opt2->prev_backup) != 0){
		return sh_cmp_nullsafe(opt1->prev_backup, opt2->prev_backup);
	}

	if (sa_cmp(opt1->directories, opt2->directories) != 0){
		return sa_cmp(opt1->directories, opt2->directories);
	}

	if (sa_cmp(opt1->exclude, opt2->exclude) != 0){
		return sa_cmp(opt1->exclude, opt2->exclude);
	}

	if (strcmp(EVP_MD_name(opt1->hash_algorithm), EVP_MD_name(opt2->hash_algorithm)) != 0){
		return strcmp(EVP_MD_name(opt1->hash_algorithm), EVP_MD_name(opt2->hash_algorithm));
	}

	if (strcmp(EVP_CIPHER_name(opt1->enc_algorithm), EVP_CIPHER_name(opt2->enc_algorithm)) != 0){
		return strcmp(EVP_CIPHER_name(opt1->enc_algorithm), EVP_CIPHER_name(opt2->enc_algorithm));
	}

	if (sh_cmp_nullsafe(opt1->enc_password, opt2->enc_password) != 0){
		return sh_cmp_nullsafe(opt1->enc_password, opt2->enc_password);
	}

	if (opt1->comp_algorithm != opt2->comp_algorithm){
		return (long)opt1->comp_algorithm - (long)opt2->comp_algorithm;
	}

	if (opt1->comp_level != opt2->comp_level){
		return opt1->comp_level - opt2->comp_level;
	}

	if (sh_cmp_nullsafe(opt1->output_directory, opt2->output_directory) != 0){
		return sh_cmp_nullsafe(opt1->output_directory, opt2->output_directory);
	}

	if (co_cmp(opt1->cloud_options, opt2->cloud_options) != 0){
		return co_cmp(opt1->cloud_options, opt2->cloud_options);
	}

	if (opt1->flags.dword != opt2->flags.dword){
		return (long)opt1->flags.dword - (long)opt2->flags.dword;
	}

	return 0;
}

int get_last_backup_file(char** out){
	struct passwd* pw;
	char* homedir;

	/* get home directory */
	if (!(homedir = getenv("HOME"))){
		pw = getpwuid(getuid());
		if (!pw){
			log_error("Failed to get home directory");
			return -1;
		}
		homedir = pw->pw_dir;
	}
	*out = malloc(strlen(homedir) + sizeof("/.ezbackup.conf"));
	if (!(*out)){
		log_enomem();
		return -1;
	}

	strcpy(*out, homedir);
	strcat(*out, "/.ezbackup.conf");

	return 0;
}

int set_last_backup_dir(const char* dir){
	char* home_conf = NULL;
	FILE* fp_conf = NULL;
	int ret = 0;

	if (get_last_backup_file(&home_conf) != 0){
		log_debug("Failed to determine home directory");
		ret = -1;
		goto cleanup;
	}

	fp_conf = fopen(home_conf, "wb");
	if (!fp_conf){
		log_efopen(home_conf);
		ret = -1;
		goto cleanup;
	}

	fwrite(dir, 1, strlen(dir), fp_conf);
	if (ferror(fp_conf) != 0){
		log_efwrite(home_conf);
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (fp_conf && fclose(fp_conf) != 0){
		log_efclose(home_conf);
	}
	free(home_conf);
	return ret;
}

int get_last_backup_dir(char** out){
	char buf[BUFFER_LEN];
	int len;
	char* home_conf = NULL;
	FILE* fp_conf = NULL;
	int ret = 0;

	if (get_last_backup_file(&home_conf) != 0){
		log_debug("Failed to determine home directory");
		ret = -1;
		goto cleanup;
	}

	fp_conf = fopen(home_conf, "wb");
	if (!fp_conf){
		log_efopen(home_conf);
		ret = -1;
		goto cleanup;
	}

	*out = sh_new();
	while ((len = read_file(fp_conf, (unsigned char*)buf, sizeof(buf) - 1)) > 0){
		buf[len] = '\0';
		*out = sh_concat(*out, buf);
	}

	if (!(*out)){
		log_debug("Failed to write to output string");
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (fp_conf && fclose(fp_conf) != 0){
		log_efclose(home_conf);
	}
	free(home_conf);
	return ret;
}

enum OPERATION parse_options_menu(struct options** opt){
	*opt = options_new();
	if (!(*opt)){
		log_error("Failed to create new options structure");
		return OP_INVALID;
	}
	return menu_main(*opt);
}
