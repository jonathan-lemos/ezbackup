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

#include "crypt.h"

#include "cli.h"
/* printf */
#include <stdio.h>
/* strcmp */
#include <string.h>
/* malloc */
#include <stdlib.h>
/* menus */
#include <ncurses.h>
#include <menu.h>
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

	if ((out = get_default_options()) != 0){
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
			out->flag_verbose = 0;
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

static int menu_compression_level(struct options* opt){
	int res;
	const char* options_compression_level[] = {
		"Default",
		"1 (fastest, lowest compression)",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9 (slowest, highest compression)"
	};

	res = display_menu(options_compression_level, ARRAY_SIZE(options_compression_level), "Select a compression level");
	opt->comp_level = res;
	return 0;
}

static int menu_compressor(struct options* opt){
	int res;
	const char* options_compressor[] = {
		"gzip  (default)",
		"bzip2 (higher compression, slower)",
		"xz    (highest compression, slowest)",
		"lz4   (fastest, lowest compression)",
		"none",
		"Exit"
	};
	enum COMPRESSOR list_compressor[] = {
		COMPRESSOR_GZIP,
		COMPRESSOR_BZIP2,
		COMPRESSOR_XZ,
		COMPRESSOR_LZ4,
		COMPRESSOR_NONE
	};

	res = display_menu(options_compressor, ARRAY_SIZE(options_compressor), "Select a compression algorithm");
	if (res == 5){
		return 0;
	}
	opt->comp_algorithm = list_compressor[res];
	return menu_compression_level(opt);
}

static int menu_checksum(struct options* opt){
	int res;
	const char* options_checksum[] = {
		"sha1   (default)",
		"sha256 (less collisions, slower)",
		"sha512 (lowest collisions, slowest)",
		"md5    (fastest, most collisions)",
		"none",
		"Exit"
	};
	const EVP_MD*(*list_checksum[])(void) = {
		EVP_sha1,
		EVP_sha256,
		EVP_sha512,
		EVP_md5,
		NULL
	};

	res = display_menu(options_checksum, ARRAY_SIZE(options_checksum), "Select a checksum algorithm");
	if (res == 5){
		return 0;
	}
	opt->hash_algorithm = list_checksum[res] ? (list_checksum[res])() : NULL;
	return 0;
}

static int menu_encryption(struct options* opt){
	int res_encryption = -1;
	int res_keysize = -1;
	int res_mode = -1;
	char* tmp = NULL;
	const char* options_encryption[] = {
		"AES (default)",
		"Camellia",
		"SEED",
		"Blowfish",
		"Triple DES (EDE3)",
		"none",
		"Exit"
	};
	const char* list_encryption[] = {
		"aes",
		"camellia",
		"seed",
		"bf",
		"des-ede3",
		NULL
	};
	const char* options_keysize[] = {
		"256 (default)",
		"192 (faster, less secure)",
		"128 (fastest, least secure)"
	};
	const char* list_keysize[] = {
		"-256",
		"-192",
		"-128"
	};
	const char* options_mode[] = {
		"Cipher Block Chaining (CBC) (default)",
		"Cipher Feedback (CFB)",
		"Output Feedback (OFB)",
		"Counter (CTR)",
	};
	const char* list_mode[] = {
		"-cbc",
		"-cfb",
		"-ofb",
		"-ctr"
	};

	res_encryption = display_menu(options_encryption, ARRAY_SIZE(options_encryption), "Select an encryption algorithm");
	if (res_encryption == 6){
		return 0;
	}
	if (res_encryption <= 1){
		res_keysize = display_menu(options_keysize, ARRAY_SIZE(options_keysize), "Select a key size");
	}
	if (res_encryption <= 2){
		res_mode = display_menu(options_mode, ARRAY_SIZE(options_mode), "Select an encryption mode");
	}
	else if (res_encryption <= 4){
		res_mode = display_menu(options_mode, ARRAY_SIZE(options_mode) - 1, "Select an encryption mode");
	}

	if (res_encryption < 0 || res_encryption == 5){
		opt->enc_algorithm = EVP_enc_null();
		return 0;
	}
	tmp = malloc(sizeof("camellia-256-cbc"));
	strcpy(tmp, list_encryption[res_encryption]);
	if (res_keysize >= 0){
		strcat(tmp, list_keysize[res_keysize]);
	}
	if (res_mode >= 0){
		strcat(tmp, list_mode[res_mode]);
	}
	opt->enc_algorithm = EVP_get_cipherbyname(tmp);
	free(tmp);

	return 0;
}

int menu_enc_password(struct options* opt){
	int res;
	char buf[1024];
	printf("Enter nothing to clear\n");

	while ((res = crypt_getpassword("Enter password:", "Verify password:", buf, sizeof(buf))) > 0){
		printf("The passwords do not match");
	}

	if (res < 0){
		log_error("Failed to read password");
		return -1;
	}

	if (strcmp(buf, "") == 0){
		if (opt->enc_password){
			free(opt->enc_password);
		}
		opt->enc_password = NULL;
		return 0;
	}
	else{
		if (opt->enc_password){
			free(opt->enc_password);
		}
		opt->enc_password = malloc(strlen(buf) + 1);
	}

	crypt_scrub(buf, strlen(buf));
	return 0;
}

int menu_directories(struct options* opt){
	int res;
	const char** options_initial = NULL;
	const char* options_dialog[] = {
		"OK"
	};

	/* TODO: refactor */
	do{
		size_t i;

		options_initial = malloc((opt->directories->len + 2) * sizeof(*(opt->directories)));
		if (!options_initial){
			log_enomem();
			return -1;
		}
		options_initial[0] = "Add a directory";
		options_initial[1] = "Exit";
		for (i = 2; i < opt->directories->len + 2; ++i){
			options_initial[i] = opt->directories->strings[i - 2];
		}

		res = display_menu(options_initial, opt->directories->len + 2, "Directories");

		switch (res){
			char* str;
			int res_confirm;
			char** options_confirm;
		case 0:
			str = readline("Enter directory:");
			if (strcmp(str, "") != 0 && sa_add(opt->directories, str) != 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Failed to add string to directory list");
				return -1;
			}
			free(str);
			if ((res = sa_sanitize_directories(opt->directories)) > 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Directory specified was invalid");
			}
			else if (res < 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Warning: Failed to sanitize directory list");
			}
			break;
		case 1:
			break;
		default:
			options_confirm = malloc(2 * sizeof(*options_confirm));
			if (!options_confirm){
				log_enomem();
				return -1;
			}
			options_confirm[0] = malloc(sizeof("Remove ") + strlen(opt->directories->strings[res - 2]));
			if (!options_confirm){
				log_enomem();
				return -1;
			}
			strcpy(options_confirm[0], "Remove ");
			strcat(options_confirm[0], opt->directories->strings[res - 2]);
			options_confirm[1] = "Exit";
			res_confirm = display_menu((const char**)options_confirm, 2, "Removing directory");

			if (res_confirm == 0){
				if (sa_remove(opt->directories, res - 2) != 0){
					log_debug("Failed to remove_directory()");
					return -1;
				}
			}

			free(options_confirm[0]);
			free(options_confirm);
			break;
		}
	}while (res != 1);
	return 0;
}

int menu_exclude(struct options* opt){
	int res;
	const char** options_initial = NULL;
	const char* title = "Exclude Paths";

	/* TODO: refactor */
	do{
		size_t i;

		options_initial = malloc((opt->exclude->len + 2) * sizeof(*(opt->exclude)));
		if (!options_initial){
			log_enomem();
			return -1;
		}
		options_initial[0] = "Add an exclude path";
		options_initial[1] = "Exit";
		for (i = 2; i < opt->exclude->len + 2; ++i){
			options_initial[i] = opt->exclude->strings[i - 2];
		}

		res = display_menu(options_initial, opt->exclude->len + 2, title);

		switch (res){
			char* str;
			int res_confirm;
			char** options_confirm;
		case 0:
			str = readline("Enter exclude path:");
			if (strcmp(str, "") != 0 && sa_add(opt->exclude, str) != 0){
				log_debug("Failed to add string to exclude list");
				return -1;
			}
			free(str);
			if ((res = sa_sanitize_directories(opt->exclude)) > 0){
				title = "Exclude path specified was invalid";
			}
			else if (res < 0){
				log_warning("Failed to sanitize exclude list");
			}
			else{
				title = "Exclude paths";
			}
			break;
		case 1:
			break;
		default:
			options_confirm = malloc(2 * sizeof(*options_confirm));
			if (!options_confirm){
				log_enomem();
				return -1;
			}
			options_confirm[0] = malloc(sizeof("Remove ") + strlen(opt->exclude->strings[res - 2]));
			if (!options_confirm){
				log_enomem();
				return -1;
			}
			strcpy(options_confirm[0], "Remove ");
			strcat(options_confirm[0], opt->exclude->strings[res - 2]);
			options_confirm[1] = "Exit";
			res_confirm = display_menu((const char**)options_confirm, 2, "Removing exclude path");

			if (res_confirm == 0){
				if (sa_remove(opt->exclude, res - 2) != 0){
					log_debug("Failed to remove_string()");
					return -1;
				}
			}

			free(options_confirm[0]);
			free(options_confirm);
			break;
		}
	}while (res != 1);
	return 0;
}

int menu_output_directory(struct options* opt){
	char* tmp;
	if (opt->output_directory){
		printf("Old directory: %s\n", opt->output_directory);
		free(opt->output_directory);
	}
	tmp = readline("Enter the output directory:");
	if (strcmp(tmp, "") == 0){
		free(tmp);
	}
	else{
		opt->output_directory = tmp;
	}
	return 0;
}

int menu_cloud_provider(struct options* opt){
	int res;
	const char* options_cp[] = {
		"None",
		"mega.nz",
		"Exit"
	};

	res = display_menu(options_cp, ARRAY_SIZE(options_cp), "Choose a cloud provider");
	switch (res){
	case 0:
		opt->cloud_options->cp = CLOUD_NONE;
		break;
	case 1:
		opt->cloud_options->cp = CLOUD_MEGA;
		break;
	case 2:
		return 0;
	default:
		log_error("Invalid option chosen. This should never happen.");
		break;
	}
	return 0;
}

int menu_cloud_username(struct options* opt){
	char* tmp = readline("Enter username:");
	if (strcmp(tmp, "") == 0){
		free(tmp);
		co_set_username(opt->cloud_options, NULL);
	}
	return 0;
}

int menu_cloud_password(struct options* opt){
	int res;
	char buf[1024];
	printf("Enter nothing to clear\n");

	while ((res = crypt_getpassword("Enter password:", "Verify password:", buf, sizeof(buf))) > 0){
		printf("The passwords do not match");
	}
	if (res < 0){
		log_error("Failed to read password");
		return -1;
	}

	if (strcmp(buf, "") == 0){
		co_set_password(opt->cloud_options, NULL);
		return 0;
	}
	else if (co_set_password(opt->cloud_options, buf) != 0){
		log_debug("Failed to set password");
		return -1;
	}

	crypt_scrub(buf, strlen(buf));
	return 0;
}

int menu_cloud(struct options* opt){
	int res;
	const char* options_cloud_main_menu[] = {
		"Cloud Provider",
		"Username",
		"Password",
		"Exit"
	};

	if (!opt->cloud_options && (opt->cloud_options = co_new()) == NULL){
		log_debug("co_new() failed");
		return -1;
	}

	do{
		res = display_menu(options_cloud_main_menu, ARRAY_SIZE(options_cloud_main_menu), "Cloud Main Menu");
		switch (res){
		case 0:
			menu_cloud_provider(opt);
			break;
		case 1:
			menu_cloud_username(opt);
			break;
		case 2:
			menu_cloud_password(opt);
			break;
		default:
			break;
		}
	}while (res != 3);

	return 0;
}

int parse_options_new(struct options* opt){
	menu_compressor(opt);
	menu_checksum(opt);
	menu_encryption(opt);
	menu_directories(opt);
	menu_exclude(opt);
	menu_output_directory(opt);
	menu_cloud(opt);
	return 0;
}

int parse_options_menu(struct options* opt){
	int res;
	const char* options_main_menu[] = {
		"Compression",
		"Encryption",
		"Directories",
		"Exclude paths",
		"Output Directory",
		"Exit"
	};
	do{
		res = display_menu(options_main_menu, ARRAY_SIZE(options_main_menu), "Configure");
		switch (res){
		case 0:
			menu_compressor(opt);
			break;
		case 1:
			menu_encryption(opt);
			break;
		case 2:
			menu_directories(opt);
			break;
		case 3:
			menu_exclude(opt);
			break;
		case 4:
			menu_output_directory(opt);
			break;
		case 5:
			break;
		default:
			log_warning("Invalid choice");
			return 0;

		}
	}while (res != 5);
	return 0;
}

struct options* get_default_options(void){
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
	opt->flag_verbose = 1;

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
	FILE* fp;
	char* tmp;
	struct options* opt = *output;

	fp = fopen(file, "rb");
	if (!fp){
		log_efopen(file);
		return -1;
	}

	if ((opt = get_default_options()) != 0){
		log_debug("Failed to get default options");
		return -1;
	}

	fscanf(fp, "[Options]");

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
	opt->enc_password = read_file_string(fp);
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

	fscanf(fp, "\nCLOUD_PROVIDER=%d", &(opt->cp));

	fscanf(fp, "\nCLOUD_USERNAME=");
	opt->username = read_file_string(fp);
	if (strcmp(opt->username, "") == 0){
		free(opt->username);
		opt->username = NULL;
	}

	fscanf(fp, "\nCLOUD_PASSWORD=");
	opt->password = read_file_string(fp);
	if (strcmp(opt->password, "") == 0){
		free(opt->password);
		opt->password = NULL;
	}

	fscanf(fp, "\nUPLOAD_DIRECTORY=");
	opt->upload_directory = read_file_string(fp);
	if (strcmp(opt->upload_directory, "") == 0){
		free(opt->upload_directory);
		opt->upload_directory = NULL;
	}

	fscanf(fp, "\nFLAGS=%u", &(opt->flags));

	if (fclose(fp) != 0){
		log_efclose(file);
	}
	return 0;
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
	int i;

	if (!file || !opt){
		log_enull();
		return -1;
	}

	fp = fopen(file, "wb");
	if (!fp){
		log_efopen(file);
		return -1;
	}
	fprintf(fp, "[Options]");
	fprintf(fp, "\nVERSION=%s%c", PROG_VERSION, '\0');
	fprintf(fp, "\nPREV=%s%c", opt->prev_backup ? opt->prev_backup : "", '\0');
	fprintf(fp, "\nDIRECTORIES=");
	for (i = 0; i < opt->directories->len; ++i){
		fprintf(fp, "%s%c", opt->directories[i], '\0');
	}
	fputc('\0', fp);
	fprintf(fp, "\nEXCLUDE=");
	for (i = 0; i < opt->exclude->len; ++i){
		fprintf(fp, "%s%c", opt->exclude[i], '\0');
	}
	fputc('\0', fp);
	fprintf(fp, "\nCHECKSUM=%s%c", EVP_MD_name(opt->hash_algorithm), '\0');
	fprintf(fp, "\nENCRYPTION=%s%c", EVP_CIPHER_name(opt->enc_algorithm), '\0');
	fprintf(fp, "\nENC_PASSWORD=%s%c", opt->enc_password ? opt->enc_password : "", '\0');
	fprintf(fp, "\nCOMPRESSION=%d", opt->comp_algorithm);
	fprintf(fp, "\nCOMP_LEVEL=%d", opt->comp_level);
	fprintf(fp, "\nOUTPUT_DIRECTORY=%s%c", opt->output_directory ? opt->output_directory : "", '\0');
	fprintf(fp, "\nCLOUD_PROVIDER=%d", opt->cp);
	fprintf(fp, "\nCLOUD_USERNAME=%s%c", opt->username ? opt->username : "", '\0');
	fprintf(fp, "\nCLOUD_PASSWORD=%s%c", opt->password ? opt->password : "", '\0');
	fprintf(fp, "\nUPLOAD_DIRECTORY=%s%c", opt->upload_directory ? opt->upload_directory : "", '\0');
	fprintf(fp, "\nFLAGS=%u", opt->flags);

	if (fclose(fp) != 0){
		log_efclose(file);
	}
	return 0;
}

void free_options(struct options* opt){
	int i;
	/* freeing a nullptr is ok */

	free(opt->prev_backup);
	free(opt->output_directory);
	for (i = 0; i < opt->exclude->len; ++i){
		free(opt->exclude[i]);
	}
	free(opt->exclude);
	for (i = 0; i < opt->directories->len; ++i){
		free(opt->directories[i]);
	}
	free(opt->directories);
	free(opt);
}

static int does_file_exist(const char* file){
	struct stat st;

	return stat(file, &st) == 0;
}

int get_config_name(char** out){
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

int read_config_file(struct options** opt, const char* path){
	char* backup_conf;

	if (path){
		if (parse_options_fromfile(path, opt) != 0){
			log_debug_ex("Failed to parse options from %s", path);
			return -1;
		}
		return 0;
	}

	if (get_config_name(&backup_conf) != 0){
		log_debug("get_config_name() failed");
		return -1;
	}

	if (!does_file_exist(backup_conf)){
		log_info("Backup file does not exist");
		free(backup_conf);
		return 1;
	}

	if (parse_options_fromfile(backup_conf, opt) != 0){
		log_debug("Failed to parse options from file");
		free(backup_conf);
		return -1;
	}

	free(backup_conf);
	return 0;
}

int write_config_file(const struct options* opt, const char* path){
	char* backup_conf = NULL;

	if (path){
		if (write_options_tofile(path, opt) != 0){
			log_warning("Failed to write settings to file");
			return -1;
		}
		return 0;
	}
	if ((get_config_name(&backup_conf)) != 0){
		log_warning("Failed to get backup name for incremental backup settings.");
		return -1;
	}
	else{
		if ((write_options_tofile(backup_conf, opt)) != 0){
			log_warning("Failed to write settings for incremental backup.");
			free(backup_conf);
			return -1;
		}
	}
	free(backup_conf);
	return 0;
}
