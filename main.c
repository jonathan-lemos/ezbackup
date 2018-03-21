/*
 * ezbackup - an easy backup solution
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
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))


typedef struct func_params{
	TAR*    tp;
	FILE*   fp_hashes;
	FILE*   fp_hashes_prev;
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

static int extract_prev_checksums(FILE* fp_in, char* out, const char* enc_algorithm, int verbose){
	char pwbuffer[1024];
	char decrypt_template[] = "/var/tmp/decrypt_XXXXXX";
	crypt_keys fk;
	FILE* fp_decrypt;

	if (!fp_in || !out || !enc_algorithm){
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

	if ((crypt_extract_salt(fp_in, &fk)) != 0){
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

	if ((fp_decrypt = temp_file_ex(decrypt_template)) == NULL){
		log_debug(__FL__, "temp_file() for file_decrypt failed");
		return 1;
	}
	if ((crypt_decrypt_ex(fp_in, &fk, fp_decrypt, verbose, "Decrypting file...")) != 0){
		crypt_free(&fk);
		log_debug(__FL__, "crypt_decrypt() failed");
		return 1;
	}
	crypt_free(&fk);
	fclose(fp_decrypt);

	if ((tar_extract_file(decrypt_template, "/checksums", out)) != 0){
		log_debug(__FL__, "tar_extract_file() failed");
		return 1;
	}

	shred_file(decrypt_template);
	return 0;
}

static int encrypt_file(FILE* fp_in, FILE* fp_out, const char* enc_algorithm, int verbose){
	char pwbuffer[1024];
	crypt_keys fk;
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

	if ((crypt_encrypt_ex(fp_in, &fk, fp_out, verbose, "Encrypting file...")) != 0){
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
		return -1;
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

static int parse_options(int argc, char** argv, func_params* fparams){
	int res;
	const char* choices_initial[] = {
		"Backup",
		"Restore",
		"Configure"
	};

	/* if we have command line args */
	if (argc >= 2){
		int parse_res;

		parse_res = parse_options_cmdline(argc, argv, &(fparams->opt));
		if (parse_res < 0 || parse_res > argc){
			log_error(__FL__, "Failed to parse command line arguments");
			return -1;
		}
		else if (parse_res != 0){
			printf("Invalid parameter %s\n", argv[parse_res]);
			return -1;
		}
		if (fparams->opt.operation == OP_INVALID){
			printf("No operation specified. Specify one of \"backup\", \"restore\", \"configure\".\n");
			return -1;
		}
		return 0;
	}

	res = read_config_file(fparams);
	if (res < 0){
		log_error(__FL__, "Failed to parse options from file");
		return -1;
	}
	else if (res > 0){
		log_info(__FL__, "Options file does not exist");

		get_default_options(&(fparams->opt));

		if (parse_options_menu(&(fparams->opt)) != 0){
			return -1;
		}
	}

	/* else get options from menu or config file */
	res = display_menu(choices_initial, ARRAY_SIZE(choices_initial), "Main Menu");
	switch (res){
	case 0:
		fparams->opt.operation = OP_BACKUP;
		break;
	case 1:
		fparams->opt.operation = OP_RESTORE;
		break;
	case 2:
		fparams->opt.operation = OP_CONFIGURE;
		break;
	default:
		log_fatal(__FL__, "Option %d chosen of %d. This should never happen", res + 1, ARRAY_SIZE(choices_initial));
		return -1;
	}

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

	err = add_checksum_to_file(file, fparams->opt.hash_algorithm, fparams->fp_hashes, fparams->fp_hashes_prev);
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

	FILE* fp_tar = NULL;
	FILE* fp_removed = NULL;
	FILE* fp_sorted = NULL;

	char template_tar[] = "/var/tmp/tar_XXXXXX";
	char template_sorted[] = "/var/tmp/sorted_XXXXXX";
	char template_prev[] = "/var/tmp/prev_XXXXXX";

	int i;

	/* set fparams values to all NULL or 0 */
	fparams.tp = NULL;
	fparams.fp_hashes = NULL;

	log_setlevel(LEVEL_WARNING);

	/* parse command line args */
	if (parse_options(argc, argv, &fparams) != 0){
		log_debug(__FL__, "Error parsing options");
		return 1;
	}

	switch(fparams.opt.operation){
	case OP_BACKUP:
		break;
	case OP_RESTORE:
		printf("Restore not implemented yet\n");
		return 0;
	case OP_CONFIGURE:
		parse_options_menu(&(fparams.opt));
		break;
	default:
		log_fatal(__FL__, "Invalid operation specified. This should never happen");
		return 1;
	}

	/* put in /home/<user>/Backups/backup-<unixtime>.tar(.bz2)(.crypt) */
	if (!fparams.opt.file_out &&
			get_default_backup_name(&(fparams.opt)) != 0){
		log_error(__FL__, "Failed to create backup name");
		return 1;
	}

	/* load hashes from previous backup if it exists */
	if (fparams.opt.prev_backup){
		FILE* fp_hashes_prev;
		FILE* fp_backup_prev;

		fp_backup_prev = fopen(fparams.opt.prev_backup, "rb");
		if (!fp_backup_prev){
			log_error(__FL__, STR_EFOPEN, fparams.opt.prev_backup, strerror(errno));
			return 1;
		}

		if ((fp_hashes_prev = temp_file_ex(template_prev)) == NULL){
			log_debug(__FL__, "Failed to create file_hashes_prev");
			return 1;
		}

		fclose(fp_hashes_prev);
		if (extract_prev_checksums(fp_backup_prev, template_prev, fparams.opt.enc_algorithm, fparams.opt.flags & FLAG_VERBOSE) != 0){
			log_debug(__FL__, "Failed to extract previous checksums");
			return 1;
		}
		fp_hashes_prev = fopen(template_prev, "r+b");
		if (!fp_hashes_prev){
			log_debug(__FL__, "Failed to reopen file_hashes_prev");
			return 1;
		}

		if (fclose(fp_backup_prev) != 0){
			log_warning(__FL__, STR_EFCLOSE, fparams.opt.prev_backup);
		}

		rewind(fp_hashes_prev);
		fparams.fp_hashes_prev = fp_hashes_prev;
	}
	else{
		fparams.fp_hashes_prev = NULL;
	}

	if ((fp_tar = temp_file_ex(template_tar)) == NULL){
		log_debug(__FL__, "Failed to make temp file for tar");
	}

	/* creating the tarball */
	printf("Adding files to %s...\n", fparams.opt.file_out);
	fparams.tp = tar_create(template_tar, fparams.opt.comp_algorithm);

	/* create initial hash list */
	if ((fparams.fp_hashes = temp_file("/var/tmp/hashes_XXXXXX")) == NULL){
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
	remove(template_prev);

	/* sort checksum file and add it to our tar */
	if ((fp_sorted = temp_file_ex(template_sorted)) == NULL){
		log_warning(__FL__, "Failed to create temp file for sorted checksum list");
	}
	else{
		if (sort_checksum_file(fparams.fp_hashes, fp_sorted) != 0){
			log_warning(__FL__, "Failed to sort checksum list");
		}
		if (tar_add_fp_ex(fparams.tp, fp_sorted, "/checksums", fparams.opt.flags & FLAG_VERBOSE, "Adding checksum list...") != 0){
			log_warning(__FL__, "Failed to write checksums to backup");
		}
		fclose(fp_sorted);
		remove(template_sorted);
	}

	fp_removed = temp_file("/var/tmp/removed_XXXXXX");
	if (!fp_removed){
		log_debug(__FL__, "Failed to create removed temp file");
	}
	else{
		if (create_removed_list(fparams.fp_hashes, fp_removed) != 0){
			log_debug(__FL__, "Failed to create removed list");
		}
	}

	/* don't really care if this fails, since this will get removed anyway */
	fclose(fparams.fp_hashes);

	if (tar_add_fp_ex(fparams.tp, fp_removed, "/removed", fparams.opt.flags & FLAG_VERBOSE, "Adding removed list...") != 0){
		log_warning(__FL__, "Failed to add removed list to backup");
	}

	/* ditto above */
	fclose(fp_removed);

	if (tar_close(fparams.tp) != 0){
		log_warning(__FL__, "Failed to close tar. Data corruption possible");
	}

	/* encrypt output */
	if (fparams.opt.enc_algorithm){
		FILE* fp_out;

		fp_out = fopen(fparams.opt.file_out, "wb");
		if (!fp_out){
			log_error(__FL__, STR_EFOPEN, fparams.opt.file_out, strerror(errno));
			return 1;
		}

		if (encrypt_file(fp_tar, fp_out, fparams.opt.enc_algorithm, fparams.opt.flags & FLAG_VERBOSE) != 0){
			log_warning(__FL__, "Failed to encrypt file");
		}

		if (fclose(fp_out) != 0){
			log_warning(__FL__, "Failed to close %s (%s). Data corruption possible", fparams.opt.file_out, strerror(errno));
		}
	}

	else{
		FILE* fp_out;

		fp_out = fopen(fparams.opt.file_out, "wb");
		if (!fp_out){
			log_error(__FL__, STR_EFOPEN, fparams.opt.file_out, strerror(errno));
			return 1;
		}

		if (copy_fp(fp_tar, fp_out) != 0){
			log_warning(__FL__, "Failed to copy output to destination.");
		}

		fclose(fp_out);
	}
	remove(template_tar);

	free(fparams.opt.prev_backup);
	fparams.opt.prev_backup = fparams.opt.file_out;

	write_config_file(fparams);

	free_options(&(fparams.opt));
	return 0;
}
