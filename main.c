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
	char*   hashes_prev;
	options opt;
}func_params;

static void print_error(const char* msg, int err){
	fprintf(stderr, "%s\nReason: %s\n", msg, err_strerror(err));
}

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
	if (rename(file_old, file_new) < 0){
		FILE* fp_old;
		FILE* fp_new;
		/* file_tar and file_out are on different disks
		 * have to copy to file_out and remove file_tar */
		unsigned char buffer[BUFFER_LEN];
		int len;

		fp_old = fopen(file_old, "rb");
		if (!fp_old){
			return err_regularerror(ERR_FILE_INPUT);
		}
		fp_new = fopen(file_new, "rb");
		if (!fp_new){
			return err_regularerror(ERR_FILE_OUTPUT);
		}

		while ((len = read_file(fp_old, buffer, sizeof(buffer))) > 0){
			if (fwrite(buffer, 1, len, fp_new) != (size_t)len){
				return err_regularerror(ERR_FILE_OUTPUT);
			}
		}
		if (!fclose(fp_old)){
			return err_regularerror(ERR_FILE_INPUT);
		}
		if (!fclose(fp_new)){
			return err_regularerror(ERR_FILE_OUTPUT);
		}
	}
	return 0;
}

static int get_config_name(char** out){
	struct passwd* pw;
	char* homedir;
	char* backupdir;
	struct stat st;

	/* get home directory */
	if (!(homedir = getenv("HOME"))){
		pw = getpwuid(getuid());
		if (!pw){
			return err_errno(errno);
		}
		homedir = pw->pw_dir;
	}

	/* /home/<user>/Backups */
	backupdir = malloc(strlen(homedir) + sizeof("/Backups"));
	if (!backupdir){
		return err_regularerror(ERR_OUT_OF_MEMORY);
	}
	strcpy(backupdir, homedir);
	strcat(backupdir, "/Backups");

	if (stat(backupdir, &st) == -1){
		if (mkdir(backupdir, 0755) == -1){
			return err_errno(errno);
		}
	}

	*out = malloc(strlen(backupdir) + sizeof("/backup.conf"));
	if (!(*out)){
		return err_regularerror(ERR_OUT_OF_MEMORY);
	}

	strcpy(*out, backupdir);
	strcat(*out, "/backup.conf");

	free(backupdir);
	return 0;
}

static int get_default_backup_name(options* opt){
	struct passwd* pw;
	char* homedir;
	char* backupdir;
	char file[sizeof("/backup-") + 50];
	struct stat st;

	/* get home directory */
	if (!(homedir = getenv("HOME"))){
		pw = getpwuid(getuid());
		if (!pw){
			return err_errno(errno);
		}
		homedir = pw->pw_dir;
	}

	/* /home/<user>/Backups */
	backupdir = malloc(strlen(homedir) + sizeof("/Backups"));
	if (!backupdir){
		return err_regularerror(ERR_OUT_OF_MEMORY);
	}
	strcpy(backupdir, homedir);
	strcat(backupdir, "/Backups");

	if (stat(backupdir, &st) == -1){
		if (mkdir(backupdir, 0755) == -1){
			return err_errno(errno);
		}
	}

	/* /home/<user>/Backups/backup-<unixtime>.tar(.bz2)(.crypt) */
	opt->file_out = malloc(strlen(backupdir) + sizeof(file));
	if (!opt->file_out){
		return err_regularerror(ERR_OUT_OF_MEMORY);
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

/* runs for each file enum_files() finds */
int fun(const char* file, const char* dir, struct stat* st, void* params){
	func_params* fparams = (func_params*)params;
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

	err = add_checksum_to_file(file, fparams->opt.hash_algorithm, fparams->fp_hashes, fparams->hashes_prev);
	if (err == 1){
		return 1;
	}
	else if (err != 0){
		fprintf(stderr, "%s: %s\n", file, err_strerror(err));
	}

	if (fparams->opt.flags & FLAG_VERBOSE){
		tar_add_file_ex(fparams->tp, file, file, 1, file);
	}
	else{
		tar_add_file(fparams->tp, file);
	}

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
	/* cannot be char*, because this memory has to be writable */
	char file_hashes[] = "/var/tmp/hashes_XXXXXX";
	char file_hashes_prev[] = "/var/tmp/prev_XXXXXX";
	char file_hashes_sorted[] = "/var/tmp/sorted_XXXXXX";
	char file_tar[] = "/var/tmp/tar_XXXXXX";
	char file_final[] = "/var/tmp/final_XXXXXX";
	char file_removed[] = "/var/tmp/removed_XXXXXX";
	char compression_string[64];
	crypt_keys fk;
	TAR* tar_final;
	int parse_res;
	int i;
	int err;
	char* backup_conf;

	/* set fparams values to all NULL or 0 */
	fparams.tp = NULL;
	fparams.fp_hashes = NULL;

	/* parse command line args */
	if (argc >= 2){
		parse_res = parse_options_cmdline(argc, argv, &(fparams.opt));
		if (parse_res < 0 || parse_res > argc){
			fprintf(stderr, "Failed to parse command line arguments\n");
			return 1;
		}
		else{
			fprintf(stderr, "Invalid parameter %s\n", argv[parse_res]);
			return 1;
		}
	}
	else{
		if ((err = get_config_name(&backup_conf)) != 0){
			print_error("Could not open backup configuration file.", err);
			return 1;
		}

		err = parse_options_fromfile(backup_conf, &(fparams.opt));
		free(backup_conf);
		switch (err){
		case 0:
			break;
		case ERR_FILE_INPUT:
			err = parse_options_menu(&(fparams.opt));
			if (err != 0){
				print_error("Failed to get options from menu.", err);
				return 1;
			}
			break;
		default:
			print_error("Could not parse options from file.", err);
			return 1;
		}
	}

	/* put in /home/<user>/Backups/backup-<unixtime>.tar(.bz2)(.crypt) */
	if (!fparams.opt.file_out &&
			(err = get_default_backup_name(&(fparams.opt))) != 0){
		print_error("Could not determine backup name.", err);
	}

	/* create temporary files */
	if ((err = temp_file(file_hashes)) != 0 ||
			(err = temp_file(file_tar)) != 0 ||
			(err = temp_file(file_final)) != 0 ||
			(err = temp_file(file_hashes_prev)) != 0 ||
			(err = temp_file(file_removed)) != 0 ||
			(err = temp_file(file_hashes_sorted)) != 0
			){
		print_error("Could not create temporary file.", err);
		return 1;
	}

	fparams.fp_hashes = fopen(file_hashes, "wb");
	if (!fparams.fp_hashes){
		int e = errno;
		err_errno(e);
		print_error("Could not open temporary file.", e);
		return 1;
	}

	/* load previous backup if it exists */
	if (fparams.opt.prev_backup){
		char pwbuffer[1024];
		crypt_keys fk;
		char file_decrypt[] = "/var/tmp/decrypt_XXXXXX";

		if ((err = temp_file(file_decrypt)) != 0){
			print_error("Failed to create temporary file for decryption.", err);
		}
		if ((err = crypt_set_encryption(fparams.opt.enc_algorithm, &fk)) != 0){
			print_error("Failed to set encryption type.", err);
			return 1;
		}
		if ((err = crypt_extract_salt(fparams.opt.prev_backup, &fk)) != 0){
			print_error("Failed to extract salt from previous backup.", err);
			return 1;
		}
		if ((err = crypt_getpassword("Enter decryption password", NULL, pwbuffer, sizeof(pwbuffer))) != 0){
			print_error("Failed to read password from terminal.", err);
			return 1;
		}
		if ((err = crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, &fk)) != 0){
			crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
			print_error("Failed to generate decryption keys.", err);
			return 1;
		}
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		if ((err = crypt_decrypt(fparams.opt.prev_backup, &fk, file_decrypt)) != 0){
			crypt_free(&fk);
			print_error("Failed to decrypt previous backup.", err);
			return 1;
		}
		crypt_free(&fk);
		if ((err = tar_extract_file(file_decrypt, "/checksums", file_hashes_prev, fparams.opt.flags & FLAG_VERBOSE)) != 0){
			print_error("Failed to extract checksums from decrypted tar.", err);
			return 1;
		}
		fparams.hashes_prev = file_hashes_prev;
		remove(file_decrypt);
	}
	else{
		fparams.hashes_prev = NULL;
	}

	printf("Adding files to %s...\n", fparams.opt.file_out);
	/* creating the initial tarball */
	fparams.tp = tar_create(file_tar, COMPRESSOR_NONE);

	/* enumerate over each directory with fun() */
	for (i = 0; i < fparams.opt.directories_len; ++i){
		if (!is_directory(fparams.opt.directories[i])){
			fprintf(stderr, "%s is not a directory. Skipping.\n", fparams.opt.directories[i]);
			continue;
		}
		enum_files(fparams.opt.directories[i], fun, &fparams, error, NULL);
	}
	fclose(fparams.fp_hashes);
	tar_close(fparams.tp);

	/* create a list of removed files */
	create_removed_list(file_hashes_prev, file_removed);

	/* creating the files + hashes tarball */
	tar_final = tar_create(file_final, fparams.opt.comp_algorithm);
	sprintf(compression_string, "Compressing files with compressor %s...", compressor_to_string(fparams.opt.comp_algorithm));
	tar_add_file_ex(tar_final, file_tar, "/files", 1, compression_string);
	sort_checksum_file(file_hashes, file_hashes_sorted);
	tar_add_file_ex(tar_final, file_hashes_sorted, "/checksums", 1, "Adding checksum list...");
	tar_add_file_ex(tar_final, file_removed, "/removed", 1, "Adding removed file list...");
	tar_close(tar_final);

	/* encrypt output */
	if (fparams.opt.enc_algorithm){
		char pwbuffer[1024];

		/* disable core dumps if possible */
		if (disable_core_dumps() != 0){
			fprintf(stderr, "WARNING: Core dumps could not be disabled\n");
		}

		crypt_set_encryption(fparams.opt.enc_algorithm, &fk);
		crypt_gen_salt(&fk);

		/* PASSWORD IN MEMORY */
		while ((err = crypt_getpassword("Enter encryption password",
						"Verify encryption password",
						pwbuffer,
						sizeof(pwbuffer))) > 0){
			printf("\nPasswords do not match\n");
		}
		if (err < 0){
			print_error("Failed to read password.", err);
			crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
			return 1;
		}

		if ((err = crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, &fk)) != 0){
			crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
			print_error("Failed to generate encryption keys.", err);
			return 1;
		}
		/* don't need to scrub entire buffer, just where the password was
		 * and a little more so attackers don't know how long the password was */
		crypt_scrub((unsigned char*)pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		/* PASSWORD OUT OF MEMORY */

		if ((err = crypt_encrypt(file_final, &fk, fparams.opt.file_out)) != 0){
			crypt_free(&fk);
			print_error("Failed to encrypt file.", err);
			return 1;
		}
		/* shreds keys as well */
		crypt_free(&fk);
	}
	else{
		if ((err = rename_ex(file_final, fparams.opt.file_out)) != 0){
			print_error("Failed to move temporary file to destination.", err);
			return 1;
		}
	}

	free(fparams.opt.prev_backup);
	fparams.opt.prev_backup = fparams.opt.file_out;

	if ((err = get_config_name(&backup_conf)) != 0){
		print_error("Warning: cannot write settings for incremental backup.", err);
	}
	else{
		if ((err = write_options_tofile(backup_conf, &(fparams.opt))) != 0){
			print_error("Warning: failed to write settings for incremental backup.", err);
		}
	}
	free(backup_conf);

	free_options(&(fparams.opt));
	remove(file_tar);
	remove(file_final);
	remove(file_hashes);
	remove(file_hashes_prev);
	remove(file_hashes_sorted);
	remove(file_removed);
	return 0;
}
