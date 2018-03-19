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

static int is_directory(const char* path){
	struct stat st;

	if (!path){
		return 0;
	}

	stat(path, &st);
	return S_ISDIR(st.st_mode);
}

static int rename_ex(const char* file_old, const char* file_new){
	/* if file_old and file_new are on different disks */
	if (rename(file_old, file_new) < 0){
		FILE* fp_old;
		FILE* fp_new;
		/* file_tar and file_out are on different disks
		 * have to copy to file_out and remove file_tar */
		unsigned char buffer[BUFFER_LEN];
		int len;

		fp_old = fopen(file_old, "rb");
		if (!fp_old){
			log_error(STR_EFOPEN, file_old, strerror(errno));
			return -1;
		}
		fp_new = fopen(file_new, "rb");
		if (!fp_new){
			log_error(STR_EFOPEN, file_new, strerror(errno));
			return -1;
		}

		while ((len = read_file(fp_old, buffer, sizeof(buffer))) > 0){
			if (fwrite(buffer, 1, len, fp_new) != (size_t)len){
				log_error(STR_EFWRITE, file_new);
				return -1;
			}
		}
		if (!fclose(fp_old)){
			log_error(STR_EFCLOSE, file_old);
		}
		if (!fclose(fp_new)){
			log_error(STR_EFCLOSE, file_new);
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
			log_error("Failed to get home directory");
			return -1;
		}
		homedir = pw->pw_dir;
	}

	/* /home/<user>/Backups */
	*out = malloc(strlen(homedir) + sizeof("/Backups"));
	if (!(*out)){
		log_fatal(STR_ENOMEM);
		return -1;
	}
	strcpy(*out, homedir);
	strcat(*out, "/Backups");

	if (stat(*out, &st) == -1){
		if (mkdir(*out, 0755) == -1){
			log_error("Failed to create backup directory at %s", *out);
			free(*out);
			return -1;
		}
	}

	return 0;
}

static int get_config_name(char** out){
	char* backupdir;

	if (get_backup_directory(&backupdir) != 0){
		puts_debug("get_backup_directory() failed");
		return -1;
	}

	*out = malloc(strlen(backupdir) + sizeof("/backup.conf"));
	if (!(*out)){
		log_fatal(STR_ENOMEM);
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
		puts_debug("get_backup_directory() failed");
		return -1;
	}

	/* /home/<user>/Backups/backup-<unixtime>.tar(.bz2)(.crypt) */
	opt->file_out = malloc(strlen(backupdir) + sizeof(file));
	if (!opt->file_out){
		free(backupdir);
		log_error(STR_ENOMEM);
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

static int extract_prev_checksums(FILE* fp_in, FILE* fp_out, const char* enc_algorithm, int verbose){
	char pwbuffer[1024];
	crypt_keys fk;
	FILE* fp_decrypt;

	if (!fp_in || !fp_out || !enc_algorithm){
		log_error(STR_ENULL);
		return 1;
	}

	if (crypt_set_encryption(enc_algorithm, &fk) != 0){
		puts_debug("crypt_set_encryption() failed");
		return 1;
	}

	if ((crypt_extract_salt(fp_in, &fk)) != 0){
		puts_debug("crypt_extract_salt() failed");
		return 1;
	}

	if ((crypt_getpassword("Enter decryption password", NULL, pwbuffer, sizeof(pwbuffer))) != 0){
		puts_debug("crypt_getpassword() failed");
		return 1;
	}

	if ((crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, &fk)) != 0){
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		puts_debug("crypt_gen_keys() failed)");
		return 1;
	}
	crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);

	if ((fp_decrypt = temp_file("/var/tmp/decrypt_XXXXXX")) == NULL){
		puts_debug("temp_file() for file_decrypt failed");
		return 1;
	}
	if ((crypt_decrypt_ex(fp_in, &fk, fp_decrypt, verbose, "Decrypting file...")) != 0){
		crypt_free(&fk);
		puts_debug("crypt_decrypt() failed");
		return 1;
	}
	crypt_free(&fk);

	if ((tar_extract_file(fp_decrypt, "/checksums", fp_out)) != 0){
		puts_debug("tar_extract_file() failed");
		return 1;
	}

	fclose(fp_decrypt);
	return 0;
}

static int encrypt_file(FILE* fp_in, FILE* fp_out, const char* enc_algorithm, int verbose){
	char pwbuffer[1024];
	crypt_keys fk;
	int err;

	/* disable core dumps if possible */
	if (disable_core_dumps() != 0){
		log_warning("Core dumps could not be disabled\n");
	}

	if (crypt_set_encryption(enc_algorithm, &fk) != 0){
		puts_debug("Could not set encryption type");
		return 1;
	}

	if (crypt_gen_salt(&fk) != 0){
		puts_debug("Could not generate salt");
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
		puts_debug("crypt_getpassword() failed");
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		return 1;
	}

	if ((crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, &fk)) != 0){
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		puts_debug("crypt_gen_keys() failed");
		return 1;
	}
	/* don't need to scrub entire buffer, just where the password was
	 * and a little more so attackers don't know how long the password was */
	crypt_scrub((unsigned char*)pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
	/* PASSWORD OUT OF MEMORY */

	if ((crypt_encrypt_ex(fp_in, &fk, fp_out, verbose, "Encrypting file...")) != 0){
		crypt_free(&fk);
		puts_debug("crypt_encrypt() failed");
		return 1;
	}
	/* shreds keys as well */
	crypt_free(&fk);
	return 0;
}

static int read_config_file(func_params fparams){
	char* backup_conf;

	if (get_config_name(&backup_conf) != 0){
		puts_debug("get_config_name() failed");
		return -1;
	}

	if (parse_options_fromfile(backup_conf, &(fparams.opt)) != 0){
		puts_debug("Failed to parse options from file (does it exist?)");
		free(backup_conf);
		return 1;
	}

	free(backup_conf);
	return 0;
}

int write_config_file(func_params fparams){
	char* backup_conf;

	if ((get_config_name(&backup_conf)) != 0){
		log_warning("Failed to get backup name for incremental backup settings.");
		return 1;
	}
	else{
		if ((write_options_tofile(backup_conf, &(fparams.opt))) != 0){
			log_warning("Failed to write settings for incremental backup.");
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

	err = add_checksum_to_file(file, fparams->opt.hash_algorithm, fparams->fp_hashes, fparams->fp_hashes_prev);
	if (err == 1){
		printf("Skipping unchanged (%s)\n", file);
		return 1;
	}
	else if (err != 0){
		puts_debug("add_checksum_to_file() failed");
	}

	path_in_tar = malloc(strlen(file) + sizeof("/files"));
	if (!path_in_tar){
		log_fatal(STR_ENOMEM);
		return 0;
	}
	strcpy(path_in_tar, "/files");
	strcat(path_in_tar, file);

	if (tar_add_file_ex(fparams->tp, file, path_in_tar, fparams->opt.flags & FLAG_VERBOSE, file) != 0){
		puts_debug("Failed to add file to tar");
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
	FILE* fp_hashes = NULL;
	FILE* fp_sorted = NULL;
	FILE* fp_removed = NULL;

	TAR* tar_final;
	int i;

	/* set fparams values to all NULL or 0 */
	fparams.tp = NULL;
	fparams.fp_hashes = NULL;

	log_setlevel(LEVEL_WARNING);

	/* parse command line args */
	if (argc >= 2){
		int parse_res;

		parse_res = parse_options_cmdline(argc, argv, &(fparams.opt));
		if (parse_res < 0 || parse_res > argc){
			log_error("Failed to parse command line arguments");
			return 1;
		}
		else{
			log_error("Invalid parameter %s\n", argv[parse_res]);
			return 1;
		}
	}
	/* get options from menu or config file */
	else{
		int res;

		res = read_config_file(fparams);
		if (res < 0){
			return 1;
		}
		else if (res > 0){
			if (parse_options_menu(&(fparams.opt)) != 0){
				log_error("Failed to parse options from menu");
				return 1;
			}
		}
	}

	/* put in /home/<user>/Backups/backup-<unixtime>.tar(.bz2)(.crypt) */
	if (!fparams.opt.file_out &&
			get_default_backup_name(&(fparams.opt)) != 0){
		log_error("Failed to create backup name");
		return 1;
	}

	/* load hashed from previous backup if it exists */
	if (fparams.opt.prev_backup){
		FILE* fp_hashes_prev;
		FILE* fp_backup_prev;

		fp_backup_prev = fopen(fparams.opt.prev_backup, "rb");
		if (!fp_backup_prev){
			log_error(STR_EFOPEN, fparams.opt.prev_backup, strerror(errno));
			return 1;
		}

		if (temp_file("/var/tmp/prev_XXXXXX") == NULL){
			puts_debug("Failed to create file_hashes_prev");
			return 1;
		}

		if (extract_prev_checksums(fp_backup_prev, fp_hashes_prev, fparams.opt.enc_algorithm, fparams.opt.flags & FLAG_VERBOSE) != 0){
			puts_debug("Failed to extract previous checksums");
			return 1;
		}

		if (fclose(fp_backup_prev) != 0){
			log_warning(STR_EFCLOSE, fparams.opt.prev_backup);
		}
		fparams.fp_hashes_prev = fp_hashes_prev;
	}
	else{
		fparams.fp_hashes_prev = NULL;
	}

	/* creating the tarball */
	printf("Adding files to %s...\n", fparams.opt.file_out);
	fparams.tp = tar_create(fparams.opt.file_out, fparams.opt.comp_algorithm);

	/* create initial hash list */
	if ((fparams.fp_hashes = temp_file("/var/tmp/hashes_XXXXXX")) == NULL){
		puts_debug("Failed to create temp file for hashes");
		return 1;
	}

	/* enumerate over each directory with fun() */
	for (i = 0; i < fparams.opt.directories_len; ++i){
		if (!is_directory(fparams.opt.directories[i])){
			log_warning("%s is not a directory. Skipping.\n", fparams.opt.directories[i]);
			continue;
		}
		enum_files(fparams.opt.directories[i], fun, &fparams, error, NULL);
	}

	if (tar_add_fp_ex(fparams.tp, fparams.fp_hashes, "/checksums", fparams.opt.flags & FLAG_VERBOSE, "Adding checksum list...") != 0){
		log_warning("Failed to write checksums to backup");
	}

	fp_removed = temp_file("/var/tmp/removed_XXXXXX");
	if (!fp_removed){
		puts_debug("Failed to create removed temp file");
	}
	else{
		if (create_removed_list(fparams.fp_hashes, fp_removed) != 0){
			puts_debug("Failed to create removed list");
		}
	}

	/* don't really care if this fails, since this will get removed anyway */
	fclose(fparams.fp_hashes);

	if (tar_add_fp_ex(fparams.tp, fp_removed, "/removed", fparams.opt.flags & FLAG_VERBOSE, "Adding removed list...") != 0){
		log_warning("Failed to add removed list to backup");
	}

	/* ditto above */
	fclose(fp_removed);

	if (tar_close(fparams.tp) != 0){
		log_warning("Failed to close tar. Data corruption possible");
	}

	/* encrypt output */
	if (fparams.opt.enc_algorithm){
		if (encrypt_file(fp_tar, fparams) != 0){
			return 1;
		}
	}
	else{
		if ((rename_ex(file_final, fparams.opt.file_out)) != 0){
			puts_debug("rename_ex() failed");
			return 1;
		}
	}

	free(fparams.opt.prev_backup);
	fparams.opt.prev_backup = fparams.opt.file_out;

	write_config_file(fparams);

	free_options(&(fparams.opt));
	remove(file_tar);
	remove(file_final);
	remove(file_hashes);
	remove(file_hashes_sorted);
	remove(file_removed);
	return 0;
}
