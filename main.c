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

#define UNUSED(x) ((void)(x))

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

static int is_directory(const char* path){
	struct stat st;

	if (!path){
		return 0;
	}

	stat(path, &st);
	return S_ISDIR(st.st_mode);
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

static int get_backup_directory(char** out){
	struct passwd* pw;
	char* homedir;
	struct stat st;

	/* get home directory */
	if (!(homedir = getenv("HOME"))){
		pw = getpwuid(getuid());
		if (!pw){
			log_error(__FL__, "Failed to get home directory");
			return -1;
		}
		homedir = pw->pw_dir;
	}

	/* /home/<user>/Backups */
	*out = malloc(strlen(homedir) + sizeof("/Backups"));
	if (!(*out)){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}
	strcpy(*out, homedir);
	strcat(*out, "/Backups");

	if (stat(*out, &st) == -1){
		if (mkdir(*out, 0755) == -1){
			log_error(__FL__, "Failed to create backup directory at %s", *out);
			free(*out);
			return -1;
		}
	}

	return 0;
}

static int get_config_name(char** out){
	char* backupdir;

	if (get_backup_directory(&backupdir) != 0){
		log_debug(__FL__, "get_backup_directory() failed");
		return -1;
	}

	*out = malloc(strlen(backupdir) + sizeof("/backup.conf"));
	if (!(*out)){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}

	strcpy(*out, backupdir);
	strcat(*out, "/backup.conf");

	free(backupdir);
	return 0;
}

static int get_default_backup_name(options* opt){
	char* backupdir;
	char file[sizeof("/backup-") + 50];

	if (get_backup_directory(&backupdir) != 0){
		log_debug(__FL__, "get_backup_directory() failed");
		return -1;
	}

	/* /home/<user>/Backups/backup-<unixtime>.tar(.bz2)(.crypt) */
	opt->file_out = malloc(strlen(backupdir) + sizeof(file));
	if (!opt->file_out){
		free(backupdir);
		log_error(__FL__, STR_ENOMEM);
		return -1;
	}

	/* get unix time and concatenate it to the filename */
	sprintf(file, "/backup-%ld", (long)time(NULL));
	strcpy(opt->file_out, backupdir);
	strcat(opt->file_out, file);

	/* concatenate extensions */
	strcat(opt->file_out, ".tar");
	switch (opt->comp_algorithm){
	case COMPRESSOR_GZIP:
		strcat(opt->file_out, ".gz");
		break;
	case COMPRESSOR_BZIP2:
		strcat(opt->file_out, ".bz2");
		break;
	case COMPRESSOR_XZ:
		strcat(opt->file_out, ".xz");
		break;
	case COMPRESSOR_LZ4:
		strcat(opt->file_out, ".lz4");
		break;
	default:
		;
	}
	if (opt->enc_algorithm){
		strcat(opt->file_out, ".crypt");
	}
	free(backupdir);
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

static int read_config_file(func_params* fparams){
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

	if (parse_options_fromfile(backup_conf, &(fparams->opt)) != 0){
		log_debug(__FL__, "Failed to parse options from file");
		free(backup_conf);
		return 1;
	}

	free(backup_conf);
	return 0;
}

int write_config_file(func_params fparams){
	char* backup_conf;

	if ((get_config_name(&backup_conf)) != 0){
		log_warning(__FL__, "Failed to get backup name for incremental backup settings.");
		return 1;
	}
	else{
		if ((write_options_tofile(backup_conf, &(fparams.opt))) != 0){
			log_warning(__FL__, "Failed to write settings for incremental backup.");
			free(backup_conf);
			return 1;
		}
	}
	free(backup_conf);
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

	err = add_checksum_to_file(file, fparams->opt.hash_algorithm, fparams->tfp_hashes->fp, fparams->fp_hashes_prev->fp);
	if (err == 1){
		/*
		   if (fparams->opt.flags & FLAG_VERBOSE){
		   printf("Skipping unchanged (%s)\n", file);
		   }
		   */
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

int main(int argc, char** argv){
	func_params fparams;

	struct TMPFILE* tfp_tar = NULL;
	struct TMPFILE* tfp_removed = NULL;
	struct TMPFILE* tfp_sorted = NULL;

	int i;

	/* set fparams values to all NULL or 0 */
	fparams.tp = NULL;
	fparams.tfp_hashes = NULL;

	log_setlevel(LEVEL_WARNING);

	/* parse command line args */
	if (argc >= 2){
		int parse_res;

		parse_res = parse_options_cmdline(argc, argv, &(fparams.opt));
		if (parse_res < 0 || parse_res > argc){
			log_error(__FL__, "Failed to parse command line arguments");
			return 1;
		}
		else{
			log_error(__FL__, "Invalid parameter %s\n", argv[parse_res]);
			return 1;
		}
	}
	/* get options from menu or config file */
	else{
		int res;

		res = read_config_file(&fparams);
		if (res < 0){
			return 1;
		}
		else if (res > 0){
			if (parse_options_menu(&(fparams.opt)) != 0){
				log_error(__FL__, "Failed to parse options from menu");
				return 1;
			}
		}
	}


	/* put in /home/<user>/Backups/backup-<unixtime>.tar(.bz2)(.crypt) */
	if (!fparams.opt.file_out &&
			get_default_backup_name(&(fparams.opt)) != 0){
		log_error(__FL__, "Failed to create backup name");
		return 1;
	}

	/* load hashes from previous backup if it exists */
	if (fparams.opt.prev_backup){
		struct TMPFILE* tfp_hashes_prev;

		if ((tfp_hashes_prev = temp_fopen("/var/tmp/prev_XXXXXX", "w+b")) == NULL){
			log_debug(__FL__, "Failed to create file_hashes_prev");
			return 1;
		}

		if (extract_prev_checksums(fparams.opt.prev_backup, tfp_hashes_prev->name, fparams.opt.enc_algorithm, fparams.opt.flags & FLAG_VERBOSE) != 0){
			log_debug(__FL__, "Failed to extract previous checksums");
			return 1;
		}

		fparams.tfp_hashes_prev = tfp_hashes_prev;
	}
	else{
		fparams.tfp_hashes_prev = NULL;
	}

	if ((tfp_tar = temp_fopen("/var/tmp/tar_XXXXXX", "wb")) == NULL){
		log_debug(__FL__, "Failed to make temp file for tar");
	}

	/* creating the tarball */
	printf("Adding files to %s...\n", fparams.opt.file_out);
	fparams.tp = tar_create(tfp_tar->name, fparams.opt.comp_algorithm);

	/* create initial hash list */
	if ((fparams.tfp_hashes = temp_fopen("/var/tmp/hashes_XXXXXX", "w+b")) == NULL){
		log_debug(__FL__, "Failed to create temp file for hashes");
		return 1;
	}

	/* enumerate over each directory with fun() */
	for (i = 0; i < fparams.opt.directories_len; ++i){
		if (!is_directory(fparams.opt.directories[i])){
			log_warning(__FL__, "%s is not a directory. Skipping.\n", fparams.opt.directories[i]);
			continue;
		}
		enum_files(fparams.opt.directories[i], fun, &fparams, error, NULL);
	}
	temp_fclose(fparams.tfp_hashes_prev);

	/* sort checksum file and add it to our tar */
	if ((tfp_sorted = temp_fopen("/var/tmp/template_sorted_XXXXXX", "wb")) == NULL){
		log_warning(__FL__, "Failed to create temp file for sorted checksum list");
	}
	else{
		if (sort_checksum_file(fparams.tfp_hashes->fp, tfp_sorted->fp) != 0){
			log_warning(__FL__, "Failed to sort checksum list");
		}
		if (tar_add_file_ex(fparams.tp, tfp_sorted->name, "/checksums", fparams.opt.flags & FLAG_VERBOSE, "Adding checksum list...") != 0){
			log_warning(__FL__, "Failed to write checksums to backup");
		}
		temp_fclose(tfp_sorted);
	}

	tfp_removed = temp_fopen("/var/tmp/removed_XXXXXX", "wb");
	if (!tfp_removed){
		log_debug(__FL__, "Failed to create removed temp file");
	}
	else{
		if (create_removed_list(fparams.tfp_hashes->fp, tfp_removed->fp) != 0){
			log_debug(__FL__, "Failed to create removed list");
		}
	}

	temp_fclose(fparams.tfp_hashes);

	if (tar_add_file_ex(fparams.tp, tfp_removed->name, "/removed", fparams.opt.flags & FLAG_VERBOSE, "Adding removed list...") != 0){
		log_warning(__FL__, "Failed to add removed list to backup");
	}

	temp_fclose(tfp_removed);

	if (tar_close(fparams.tp) != 0){
		log_warning(__FL__, "Failed to close tar. Data corruption possible");
	}

	/* encrypt output */
	if (fparams.opt.enc_algorithm &&
			encrypt_file(tfp_tar->name, fparams.opt.file_out, fparams.opt.enc_algorithm, fparams.opt.flags & FLAG_VERBOSE) != 0){
			log_warning(__FL__, "Failed to encrypt file");
	}
	else{
		FILE* fp_out;

		fp_out = fopen(fparams.opt.file_out, "wb");
		if (!fp_out){
			log_error(__FL__, STR_EFOPEN, fparams.opt.file_out, strerror(errno));
			return 1;
		}

		if (copy_fp(tfp_tar->fp, fp_out) != 0){
			log_warning(__FL__, "Failed to copy output to destination.");
		}

		fclose(fp_out);
	}

	temp_fclose(tfp_tar);

	free(fparams.opt.prev_backup);
	fparams.opt.prev_backup = fparams.opt.file_out;

	write_config_file(fparams);

	free_options(&(fparams.opt));
	return 0;
}
