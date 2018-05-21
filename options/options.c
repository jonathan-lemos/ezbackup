/* options.c -- command-line/menu-based options parser
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* prototypes */
#include "options.h"
/* errors */
#include "error.h"
#include <errno.h>

#include "filehelper.h"

#include "crypt/base64.h"

#include "stringhelper.h"
/* printf */
#include <stdio.h>
/* strcmp */
#include <string.h>
/* malloc */
#include <stdlib.h>

/* char* to EVP_MD(*)(void) */
#include <openssl/evp.h>
/* command-line tab completion */
#if defined(__linux__)
#include <editline/readline.h>
#elif defined(__APPLE__)
#include <readline/readline.h>
#else
#error "This operating system is not supported"
#endif
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

	return_ifnull(out, ((int(*)(void))abort)());

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
	opt->comp_algorithm = COMPRESSOR_GZIP;
	opt->comp_level = 0;
	if (get_backup_directory(&(opt->output_directory)) != 0){
		log_debug("Failed to make backup directory");
		return NULL;
	}
	opt->cloud_options = co_new();
	opt->flags.bits.flag_verbose = 1;

	return opt;
}

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

int parse_options_fromfile(const char* file, struct options** output){
	FILE* fp = NULL;
	char* tmp;
	struct options* opt = *output;
	int ret = 0;

	fp = fopen(file, "rb");
	if (!fp){
		log_efopen(file);
		ret = -1;
		goto cleanup;
	}

	if ((opt = options_new()) != 0){
		log_debug("Failed to get default options");
		ret = -1;
		goto cleanup;
	}

	fscanf(fp, "[Options]");
	fscanf(fp, "\nVERSION=%*s");
	fscanf(fp, "\nPREV=");
	opt->prev_backup = read_file_string(fp);
	if (strcmp(opt->prev_backup, "") == 0){
		free(opt->prev_backup);
		opt->prev_backup = NULL;
	}

	fscanf(fp, "\nDIRECTORIES=");
	do{
		tmp = read_file_string(fp);
		if (!tmp){
			break;
		}
		if (tmp[0] != '\0'){
			sa_add(opt->directories, tmp);
			free(tmp);
		}
		else{
			free(tmp);
			tmp = NULL;
		}
	}while (tmp);
	fscanf(fp, "\nEXCLUDE=");
	do{
		tmp = read_file_string(fp);
		if (!tmp){
			break;
		}
		if (tmp[0] != '\0'){
			sa_add(opt->exclude, tmp);
			free(tmp);
		}
		else{
			free(tmp);
			tmp = NULL;
		}
	}while (tmp);

	fscanf(fp, "\nCHECKSUM=");
	tmp = read_file_string(fp);
	if (!tmp){
		opt->hash_algorithm = NULL;
	}
	else{
		opt->hash_algorithm = EVP_get_digestbyname(tmp);
		free(tmp);
	}

	fscanf(fp, "\nENCRYPTION=");
	tmp = read_file_string(fp);
	if (!tmp){
		opt->enc_algorithm = EVP_enc_null();
	}
	else{
		opt->enc_algorithm = EVP_get_cipherbyname(tmp);
		free(tmp);
	}

	fscanf(fp, "\nENC_PASSWORD=");
	tmp = read_file_string(fp);
	opt->enc_password = (char*)from_base64((unsigned char*)tmp, strlen(tmp), NULL);
	if (strcmp(opt->enc_password, "") == 0){
		free(opt->enc_password);
		opt->enc_password = NULL;
	}

	fscanf(fp, "\nCOMPRESSION=%d", &(opt->comp_algorithm));

	fscanf(fp, "\nCOMP_LEVEL=%d", &(opt->comp_level));

	fscanf(fp, "\nOUTPUT_DIRECTORY=");
	opt->output_directory = read_file_string(fp);
	if (strcmp(opt->output_directory, "") == 0){
		free(opt->output_directory);
		opt->output_directory = NULL;
	}

	fscanf(fp, "\nCLOUD_PROVIDER=%d", &(opt->cloud_options->cp));

	fscanf(fp, "\nCLOUD_USERNAME=");
	opt->cloud_options->username = read_file_string(fp);
	if (strcmp(opt->cloud_options->username, "") == 0){
		free(opt->cloud_options->username);
		opt->cloud_options->username = NULL;
	}

	fscanf(fp, "\nCLOUD_PASSWORD=");
	opt->cloud_options->password = read_file_string(fp);
	if (strcmp(opt->cloud_options->password, "") == 0){
		free(opt->cloud_options->password);
		opt->cloud_options->password = NULL;
	}

	fscanf(fp, "\nCLOUD_DIRECTORY=");
	opt->cloud_options->upload_directory = read_file_string(fp);
	if (strcmp(opt->cloud_options->upload_directory, "") == 0){
		free(opt->cloud_options->upload_directory);
		opt->cloud_options->upload_directory = NULL;
	}

	fscanf(fp, "\nFLAGS=%u", &(opt->flags.dword));

cleanup:
	if (fclose(fp) != 0){
		log_efclose(file);
	}
	return ret;
}

/* File format:
 *
 * [Options]
 * PREV=/path/to/prev.tar\0
 * DIRECTORIES=/dir1\0/dir2\0/dir3\0\0
 * EXCLUDE=/exc1\0/exc2\0/exc3\0\0
 * CHECKSUM=sha1\0
 * ENCRYPTION=aes-256-cbc\0
 * COMPRESSION=2
 * FLAGS=1
 *
 */
int write_options_tofile(const char* file, const struct options* opt){
	FILE* fp;
	size_t i;

	if (!file || !opt){
		log_enull();
		return -1;
	}

	fp = fopen(file, "wb");
	if (!fp){
		log_efopen(file);
		fclose(fp);
		return -1;
	}
	fprintf(fp, "[Options]");
	fprintf(fp, "\nVERSION=%s%c", PROG_VERSION, '\0');
	fprintf(fp, "\nPREV=%s%c", opt->prev_backup ? opt->prev_backup : "", '\0');
	fprintf(fp, "\nDIRECTORIES=");
	for (i = 0; i < opt->directories->len; ++i){
		fprintf(fp, "%s%c", opt->directories->strings[i], '\0');
	}
	fputc('\0', fp);
	fprintf(fp, "\nEXCLUDE=");
	for (i = 0; i < opt->exclude->len; ++i){
		fprintf(fp, "%s%c", opt->exclude->strings[i], '\0');
	}
	fputc('\0', fp);
	fprintf(fp, "\nCHECKSUM=%s%c", EVP_MD_name(opt->hash_algorithm), '\0');
	fprintf(fp, "\nENCRYPTION=%s%c", EVP_CIPHER_name(opt->enc_algorithm), '\0');
	fprintf(fp, "\nENC_PASSWORD=%s%c", opt->enc_password ? opt->enc_password : "", '\0');
	fprintf(fp, "\nCOMPRESSION=%d", opt->comp_algorithm);
	fprintf(fp, "\nCOMP_LEVEL=%d", opt->comp_level);
	fprintf(fp, "\nOUTPUT_DIRECTORY=%s%c", opt->output_directory ? opt->output_directory : "", '\0');
	fprintf(fp, "\nCLOUD_PROVIDER=%d", opt->cloud_options->cp);
	fprintf(fp, "\nCLOUD_USERNAME=%s%c", opt->cloud_options->username ? opt->cloud_options->username : "", '\0');
	fprintf(fp, "\nCLOUD_PASSWORD=%s%c", opt->cloud_options->password ? opt->cloud_options->password : "", '\0');
	fprintf(fp, "\nCLOUD_DIRECTORY=%s%c", opt->cloud_options->upload_directory ? opt->cloud_options->upload_directory : "", '\0');
	fprintf(fp, "\nFLAGS=%u", opt->flags.dword);

	if (fclose(fp) != 0){
		log_efclose(file);
	}
	return 0;
}

void free_options(struct options* opt){
	free(opt->prev_backup);
	sa_free(opt->directories);
	sa_free(opt->exclude);
	free(opt->enc_password);
	free(opt->output_directory);
	co_free(opt->cloud_options);
	free(opt);
}

static int does_file_exist(const char* file){
	struct stat st;

	return stat(file, &st) == 0;
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

	buf[len] = '\0';
	*out = sh_new();
	while ((len = read_file(fp_conf, (unsigned char*)buf, sizeof(buf) - 1)) > 0){
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
