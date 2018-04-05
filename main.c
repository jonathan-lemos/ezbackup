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


typedef struct func_params{
	TAR*            tp;
	struct TMPFILE* tfp_hashes;
	struct TMPFILE* tfp_hashes_prev;
	options opt;
}func_params;

static int disable_core_dumps(void){
	struct rlimit rl;
	rl.rlim_cur = 0;
	rl.rlim_max = 0;
	return setrlimit(RLIMIT_CORE, &rl);
}

static int copy_fp(FILE* in, FILE* out){
	unsigned char buffer[BUFFER_LEN];
	int len;

	while ((len = read_file(in, buffer, sizeof(buffer))) > 0){
		if ((int)fwrite(buffer, 1, len, out) != len){
			log_error(__FL__, STR_EFWRITE, "file");
			return 1;
		}
	}

	return 0;
}

static int get_config_name(char** out){
	struct passwd* pw;
	char* homedir;

	/* get home directory */
	if (!(homedir = getenv("HOME"))){
		pw = getpwuid(getuid());
		if (!pw){
			log_error(__FL__, "Failed to get home directory");
			return -1;
		}
		homedir = pw->pw_dir;
	}
	*out = malloc(strlen(homedir) + sizeof("/.ezbackup.conf"));
	if (!(*out)){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}

	strcpy(*out, homedir);
	strcat(*out, "/.ezbackup.conf");

	return 0;
}

static int get_default_backup_name(options* opt, char** out){
	char file[128];

	/* /home/<user>/Backups/backup-<unixtime>.tar(.bz2)(.crypt) */
	*out = malloc(strlen(opt->output_directory) + sizeof(file));
	if (!(out)){
		log_error(__FL__, STR_ENOMEM);
		return -1;
	}

	/* get unix time and concatenate it to the filename */
	sprintf(file, "/backup-%ld", (long)time(NULL));
	strcpy(*out, opt->output_directory);
	strcat(*out, file);

	/* concatenate extensions */
	strcat(*out, ".tar");
	switch (opt->comp_algorithm){
	case COMPRESSOR_GZIP:
		strcat(*out, ".gz");
		break;
	case COMPRESSOR_BZIP2:
		strcat(*out, ".bz2");
		break;
	case COMPRESSOR_XZ:
		strcat(*out, ".xz");
		break;
	case COMPRESSOR_LZ4:
		strcat(*out, ".lz4");
		break;
	default:
		;
	}
	if (opt->enc_algorithm){
		char enc_algo_str[64];
		sprintf(enc_algo_str, ".%s", EVP_CIPHER_name(opt->enc_algorithm));
		strcat(*out, enc_algo_str);
	}
	return 0;
}

static int extract_prev_checksums(const char* in, const char* out, const char* enc_algorithm, int verbose){
	char pwbuffer[1024];
	struct crypt_keys fk;
	struct TMPFILE* tfp_decrypt;

	if (!in || !out || !enc_algorithm){
		log_error(__FL__, STR_ENULL);
		return 1;
	}

	if (disable_core_dumps() != 0){
		log_warning(__FL__, "Core dumps could not be disabled");
	}

	if (crypt_set_encryption(enc_algorithm, &fk) != 0){
		log_debug(__FL__, "crypt_set_encryption() failed");
		return 1;
	}

	if ((crypt_extract_salt(in, &fk)) != 0){
		log_debug(__FL__, "crypt_extract_salt() failed");
		return 1;
	}

	if ((crypt_getpassword("Enter decryption password", NULL, pwbuffer, sizeof(pwbuffer))) != 0){
		log_debug(__FL__, "crypt_getpassword() failed");
		return 1;
	}

	if ((crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, &fk)) != 0){
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		log_debug(__FL__, "crypt_gen_keys() failed)");
		return 1;
	}
	crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);

	if ((tfp_decrypt = temp_fopen("/var/tmp/decrypt_XXXXXX", "w+b")) == NULL){
		log_debug(__FL__, "temp_file() for file_decrypt failed");
		return 1;
	}
	if ((crypt_decrypt_ex(in, &fk, out, verbose, "Decrypting file...")) != 0){
		crypt_free(&fk);
		log_debug(__FL__, "crypt_decrypt() failed");
		return 1;
	}
	crypt_free(&fk);

	if ((tar_extract_file(tfp_decrypt->name, "/checksums", out)) != 0){
		log_debug(__FL__, "tar_extract_file() failed");
		return 1;
	}

	shred_file(tfp_decrypt->name);
	if (temp_fclose(tfp_decrypt) != 0){
		log_debug(__FL__, STR_EFCLOSE);
	}
	return 0;
}

static int encrypt_file(const char* in, const char* out, const char* enc_algorithm, int verbose){
	char pwbuffer[1024];
	struct crypt_keys fk;
	int err;

	/* disable core dumps if possible */
	if (disable_core_dumps() != 0){
		log_warning(__FL__, "Core dumps could not be disabled\n");
	}

	if (crypt_set_encryption(enc_algorithm, &fk) != 0){
		log_debug(__FL__, "Could not set encryption type");
		return 1;
	}

	if (crypt_gen_salt(&fk) != 0){
		log_debug(__FL__, "Could not generate salt");
		return 1;
	}

	/* PASSWORD IN MEMORY */
	while ((err = crypt_getpassword("Enter encryption password",
					"Verify encryption password",
					pwbuffer,
					sizeof(pwbuffer))) > 0){
		printf("\nPasswords do not match\n");
	}
	if (err < 0){
		log_debug(__FL__, "crypt_getpassword() failed");
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		return 1;
	}

	if ((crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, &fk)) != 0){
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		log_debug(__FL__, "crypt_gen_keys() failed");
		return 1;
	}
	/* don't need to scrub entire buffer, just where the password was
	 * and a little more so attackers don't know how long the password was */
	crypt_scrub((unsigned char*)pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
	/* PASSWORD OUT OF MEMORY */

	if ((crypt_encrypt_ex(in, &fk, out, verbose, "Encrypting file...")) != 0){
		crypt_free(&fk);
		log_debug(__FL__, "crypt_encrypt() failed");
		return 1;
	}
	/* shreds keys as well */
	crypt_free(&fk);
	return 0;
}

static int does_file_exist(const char* file){
	struct stat st;

	return stat(file, &st) == 0;
}

int read_config_file(struct options* opt){
	char* backup_conf;

	if (get_config_name(&backup_conf) != 0){
		log_debug(__FL__, "get_config_name() failed");
		return -1;
	}

	if (!does_file_exist(backup_conf)){
		log_info(__FL__, "Backup file does not exist");
		free(backup_conf);
		return 1;
	}

	if (parse_options_fromfile(backup_conf, opt) != 0){
		log_debug(__FL__, "Failed to parse options from file");
		free(backup_conf);
		return -1;
	}

	free(backup_conf);
	return 0;
}

int write_config_file(func_params* fparams){
	char* backup_conf;

	if ((get_config_name(&backup_conf)) != 0){
		log_warning(__FL__, "Failed to get backup name for incremental backup settings.");
		return 1;
	}
	else{
		if ((write_options_tofile(backup_conf, &(fparams->opt))) != 0){
			log_warning(__FL__, "Failed to write settings for incremental backup.");
			free(backup_conf);
			return 1;
		}
	}
	free(backup_conf);
	return 0;
}

static int parse_options_prev(int argc, char** argv, options* opt){
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

/* runs for each file enum_files() finds */
int fun(const char* file, const char* dir, struct stat* st, void* params){
	func_params* fparams = (func_params*)params;
	char* path_in_tar;
	int err;
	int i;
	UNUSED(st);

	/* exclude lost+found */
	if (strlen(dir) > strlen("lost+found") &&
			!strcmp(dir + strlen(dir) - strlen("lost+found"), "lost+found")){
		return 0;
	}
	/* exclude in exclude list */
	for (i = 0; i < fparams->opt.exclude_len; ++i){
		/* stop iterating through this directory */
		if (!strcmp(dir, fparams->opt.exclude[i])){
			return 0;
		}
	}

	err = add_checksum_to_file(file, fparams->opt.hash_algorithm, fparams->tfp_hashes->fp, fparams->tfp_hashes_prev ? fparams->tfp_hashes_prev->fp : NULL);
	if (err == 1){

		if (fparams->opt.flags & FLAG_VERBOSE){
			printf("Skipping unchanged (%s)\n", file);
		}

		return 1;
	}
	else if (err != 0){
		log_debug(__FL__, "add_checksum_to_file() failed");
	}

	path_in_tar = malloc(strlen(file) + sizeof("/files"));
	if (!path_in_tar){
		log_fatal(__FL__, STR_ENOMEM);
		return 0;
	}
	strcpy(path_in_tar, "/files");
	strcat(path_in_tar, file);

	if (tar_add_file_ex(fparams->tp, file, path_in_tar, fparams->opt.flags & FLAG_VERBOSE, file) != 0){
		log_debug(__FL__, "Failed to add file to tar");
	}
	free(path_in_tar);

	return 1;
}

/* runs if enum_files() cannot open directory */
int error(const char* file, int __errno, void* params){
	UNUSED(params);
	fprintf(stderr, "%s: %s\n", file, strerror(__errno));

	return 1;
}

static int mega_create_account(char** out_username){
	(void)out_username;
	printf("Not implemented yet\n");
	sleep(3);
	return 0;
}

static int mega_upload(const char* file, const char* username, const char* password){
	char* uname = NULL;
	int res;
	MEGAhandle mh;
	const char* options_username[] = {
		"Login",
		"Create an account (not implemented yet)"
	};

	if (!username){
		res = display_menu(options_username, ARRAY_SIZE(options_username), "MEGA");
		switch (res){
		case 0:
			uname = readline("Enter username:");
			break;
		case 1:
			if (mega_create_account(&uname) != 0){
				log_debug(__FL__, "Failed to create MEGA account");
				return -1;
			}
		default:
			log_fatal(__FL__, "Option %d chosen of %d. This should never happen", res, ARRAY_SIZE(options_username));
			return -1;
		}
	}
	else{
		uname = malloc(strlen(username) + 1);
		if (!uname){
			log_fatal(__FL__, STR_ENOMEM);
			return -1;
		}
		strcpy(uname, username);
	}

	if (MEGAlogin(uname, password, &mh) != 0){
		log_debug(__FL__, "Failed to log in to MEGA");
		free(uname);
		return -1;
	}
	if (MEGAmkdir("/Backups", mh) < 0){
		log_debug(__FL__, "Failed to create backup directory in MEGA");
		free(uname);
		MEGAlogout(mh);
		return -1;
	}
	if (MEGAupload(file, "/Backups", "Uploading file to MEGA", mh) != 0){
		log_debug(__FL__, "Failed to upload file to MEGA");
		free(uname);
		MEGAlogout(mh);
		return -1;
	}
	if (MEGAlogout(mh) != 0){
		log_debug(__FL__, "Failed to logout of MEGA");
		free(uname);
		return -1;
	}
	free(uname);
	return 0;
}

static int mega_download(const char* out_file, const char* username, const char* password){
	int res;
	char** files;
	MEGAhandle mh;

	if (MEGAlogin(username, password, &mh) != 0){
		log_debug(__FL__, "Failed to login to MEGA");
		return -1;
	}
	res = MEGAreaddir("/Backups", &files, mh);
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

static int backup(func_params* fparams, const options* opt_prev){
	FILE* fp_tar = NULL;
	FILE* fp_removed = NULL;
	FILE* fp_sorted = NULL;

	char template_tar[] = "/var/tmp/tar_XXXXXX";
	char template_sorted[] = "/var/tmp/sorted_XXXXXX";
	char template_prev[] = "/var/tmp/prev_XXXXXX";

	char* file_out;
	int i;

	/* load hashes from previous backup if it exists */
	if (opt_prev && opt_prev->prev_backup && opt_prev->hash_algorithm == fparams->opt.hash_algorithm){
		FILE* fp_hashes_prev;
		FILE* fp_backup_prev;

		fp_backup_prev = fopen(opt_prev->prev_backup, "rb");
		if (!fp_backup_prev){
			log_error(__FL__, STR_EFOPEN, opt_prev->prev_backup, strerror(errno));
			return 1;
		}

		if ((tfp_hashes_prev = temp_fopen("/var/tmp/prev_XXXXXX", "w+b")) == NULL){
			log_debug(__FL__, "Failed to create file_hashes_prev");
			return 1;
		}

		fclose(fp_hashes_prev);
		if (extract_prev_checksums(fp_backup_prev, template_prev, opt_prev->enc_algorithm, opt_prev->flags & FLAG_VERBOSE) != 0){
			log_debug(__FL__, "Failed to extract previous checksums");
			return 1;
		}
		fp_hashes_prev = fopen(template_prev, "r+b");
		if (!fp_hashes_prev){
			log_debug(__FL__, "Failed to reopen file_hashes_prev");
			return 1;
		}

		if (fclose(fp_backup_prev) != 0){
			log_warning(__FL__, STR_EFCLOSE, opt_prev->prev_backup);
		}

		rewind(fp_hashes_prev);
		fparams->fp_hashes_prev = fp_hashes_prev;
	}
	else{
		fparams->fp_hashes_prev = NULL;
	}

	if ((tfp_tar = temp_fopen("/var/tmp/tar_XXXXXX", "wb")) == NULL){
		log_debug(__FL__, "Failed to make temp file for tar");
	}

	/* put in /home/<user>/Backups/backup-<unixtime>.tar(.bz2)(.crypt) */
	if (get_default_backup_name(&(fparams->opt), &file_out) != 0){
		log_error(__FL__, "Failed to create backup name");
		return 1;
	}

	/* creating the tarball */
	printf("Adding files to %s...\n", file_out);
	fparams->tp = tar_create(template_tar, fparams->opt.comp_algorithm, fparams->opt.comp_level);

	/* create initial hash list */
	if ((fparams->fp_hashes = temp_file("/var/tmp/hashes_XXXXXX")) == NULL){
		log_debug(__FL__, "Failed to create temp file for hashes");
		return 1;
	}

	/* enumerate over each directory with fun() */
	for (i = 0; i < fparams->opt.directories_len; ++i){
		enum_files(fparams->opt.directories[i], fun, fparams, error, NULL);
	}
	temp_fclose(fparams.tfp_hashes_prev);

	/* sort checksum file and add it to our tar */
	if ((tfp_sorted = temp_fopen("/var/tmp/template_sorted_XXXXXX", "wb")) == NULL){
		log_warning(__FL__, "Failed to create temp file for sorted checksum list");
	}
	else{
		if (sort_checksum_file(fparams->fp_hashes, fp_sorted) != 0){
			log_warning(__FL__, "Failed to sort checksum list");
		}
		if (tar_add_fp_ex(fparams->tp, fp_sorted, "/checksums", fparams->opt.flags & FLAG_VERBOSE, "Adding checksum list...") != 0){
			log_warning(__FL__, "Failed to write checksums to backup");
		}
		temp_fclose(tfp_sorted);
	}

	tfp_removed = temp_fopen("/var/tmp/removed_XXXXXX", "wb");
	if (!tfp_removed){
		log_debug(__FL__, "Failed to create removed temp file");
	}
	else{
		if (create_removed_list(fparams->fp_hashes, fp_removed) != 0){
			log_debug(__FL__, "Failed to create removed list");
		}
	}

	/* don't really care if this fails, since this will get removed anyway */
	fclose(fparams->fp_hashes);

	if (tar_add_fp_ex(fparams->tp, fp_removed, "/removed", fparams->opt.flags & FLAG_VERBOSE, "Adding removed list...") != 0){
		log_warning(__FL__, "Failed to add removed list to backup");
	}

	/*
	   if (get_config_name(&name_config) != 0){
	   log_warning(__FL__, "Failed to determine config name");
	   }
	   else{
	   fp_config = fopen(name_config, "rb");
	   if (!fp_config){
	   log_warning(__FL__, "Failed to open config file (%s)", strerror(errno));
	   }
	   }
	   if (fp_config){
	   if (tar_add_fp_ex(fparams->tp, fp_config, "/config", fparams->opt.flags & FLAG_VERBOSE, "Adding config...") != 0){
	   log_warning(__FL__, "Failed to add config to backup");
	   }
	   }
	   */

	/* ditto above */
	fclose(fp_removed);
	if (tar_close(fparams->tp) != 0){
		log_warning(__FL__, "Failed to close tar. Data corruption possible");
	}

	/* encrypt output */
	if (fparams->opt.enc_algorithm){
		FILE* fp_out;

		fp_out = fopen(file_out, "wb");
		if (!fp_out){
			log_error(__FL__, STR_EFOPEN, file_out, strerror(errno));
			return 1;
		}

		if (encrypt_file(fp_tar, fp_out, fparams->opt.enc_algorithm, fparams->opt.flags & FLAG_VERBOSE) != 0){
			log_warning(__FL__, "Failed to encrypt file");
		}

		if (fclose(fp_out) != 0){
			log_warning(__FL__, "Failed to close %s (%s). Data corruption possible", file_out, strerror(errno));
		}
	}
	else{
		FILE* fp_out;

		fp_out = fopen(file_out, "wb");
		if (!fp_out){
			log_error(__FL__, STR_EFOPEN, file_out, strerror(errno));
			return 1;
		}

		if (copy_fp(tfp_tar->fp, fp_out) != 0){
			log_warning(__FL__, "Failed to copy output to destination.");
		}

		fclose(fp_out);
	}

	temp_fclose(tfp_tar);

	if (mega_upload(file_out, NULL, NULL) != 0){
		log_debug(__FL__, "Failed to upload to MEGA");
		return -1;
	}

	free(fparams->opt.prev_backup);
	fparams->opt.prev_backup = file_out;

	write_config_file(fparams);

	free_options(&(fparams->opt));
	return 0;
}

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
		char* homedir;
		struct passwd* pw;
	case OP_BACKUP:
		if (!(homedir = getenv("HOME"))){
			pw = getpwuid(getuid());
			if (!pw){
				log_error(__FL__, "Failed to get home directory");
				return -1;
			}
			homedir = pw->pw_dir;
		}
		fparams.opt.directories = malloc(sizeof(*(fparams.opt.directories)));
		if (!fparams.opt.directories){
			log_fatal(__FL__, STR_ENOMEM);
			return -1;
		}
		fparams.opt.directories[0] = malloc(strlen(homedir) + 1);
		if (!fparams.opt.directories[0]){
			free(fparams.opt.directories);
			log_fatal(__FL__, STR_ENOMEM);
			return -1;
		}
		fparams.opt.directories_len = 1;
		strcpy(fparams.opt.directories[0], homedir);
		backup(&fparams, &opt_prev);
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
