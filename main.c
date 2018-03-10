/* iterates files */
#include "fileiterator.h"
/* makes the tar */
#include "maketar.h"
/* read_file() */
#include "readfile.h"
/* error handling */
#include "error.h"
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
/* reading password */
#include <termios.h>
/* getting home directory */
#include <unistd.h>
#include <pwd.h>
/* strftime */
#include <time.h>
/* disabling core dumps */
#include <sys/resource.h>

#define UNUSED(x) ((void)(x))

#define FLAG_VERBOSE (0x1)

typedef struct func_params{
	TAR*        tp;
	FILE*       fp_hashes;
	options     opt;
}func_params;

int disable_core_dumps(void){
	struct rlimit rl;
	rl.rlim_cur = 0;
	rl.rlim_max = 0;
	return setrlimit(RLIMIT_CORE, &rl);
}

int get_pass(char* out, size_t len){
}

int is_directory(const char* path){
	struct stat st;

	if (!path){
		return 0;
	}

	stat(path, &st);
	return S_ISDIR(st.st_mode);
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

	if (fparams->opt.flags & FLAG_VERBOSE){
		tar_add_file_ex(fparams->tp, file, file, 1, file);
	}
	else{
		tar_add_file(fparams->tp, file);
	}

	if ((err = add_checksum_to_file(file, fparams->opt.hash_algorithm, fparams->fp_hashes) != 0)){
		printf("%s: %s\n", file, evp_strerror(err));
	}
	return 1;
}

/* runs if enum_files() cannot open directory */
int error(const char* file, int __errno, void* params){
	UNUSED(params);
	printf("%s: %s\n", file, strerror(__errno));

	return 1;
}

int main(int argc, char** argv){
	func_params fparams;

	/* temp files */
	FILE* fp_hashes = NULL;
/*  FILE* fp_tar = NULL; */
	FILE* fp_final = NULL;
	FILE* fp_out = NULL;
	/* cannot be char*, because this memory has to be writable */
	/* temp file templates */
	char file_hashes[] = "/tmp/hashes_XXXXXX";
	char file_tar[] = "/tmp/tar_XXXXXX";
	char file_final[] = "/tmp/final_XXXXXX";

	crypt_keys fk;
	TAR* tar_final;

	int parse_res;
	int i;

	/* set fparams values to all NULL or 0 */
	fparams.tp = NULL;
	fparams.fp_hashes = NULL;

	/* parse command line args */
	if ((parse_res = parse_options_cmdline(argc, argv, &(fparams.opt))) != 0){
		if (parse_res < 0 || parse_res > argc){
			fprintf(stderr, "satan has infected the computer\n");
			return 1;
		}
		else{
			fprintf(stderr, "Invalid parameter %s\n", argv[parse_res]);
			return 1;
		}
	}

	/* put in /home/<user>/Backups/backup-unixtime.tar[.bz2][.crypt] */
	if (!fparams.opt.file_out){
		struct passwd* pw;
		char* homedir;
		char* backupdir;
		char file[sizeof("/backup-") + 20];
		struct stat st;

		if (!(homedir = getenv("HOME"))){
			pw = getpwuid(getuid());
			homedir = pw->pw_dir;
		}

		backupdir = malloc(strlen(homedir) + sizeof("/Backups"));
		strcpy(backupdir, homedir);
		strcat(backupdir, "/Backups");

		if (stat(backupdir, &st) == -1){
			if (mkdir(backupdir, 0755) == -1){
				fprintf(stderr, "Could not create %s\n", backupdir);
				return 1;
			}
		}

		fparams.opt.file_out = malloc(strlen(backupdir) + sizeof(file));
		if (!fparams.opt.file_out){
			fprintf(stderr, "System is out of memory\n");
			return 1;
		}

		/* format time by unix time */
		sprintf(file, "/backup-%ld", (long)time(NULL));
		strcpy(fparams.opt.file_out, backupdir);
		strcat(fparams.opt.file_out, file);
		strcat(fparams.opt.file_out, ".tar");
		switch (fparams.opt.comp_algorithm){
			case COMPRESSOR_GZIP:
				strcat(fparams.opt.file_out, ".gz");
				break;
			case COMPRESSOR_BZIP2:
				strcat(fparams.opt.file_out, ".bz2");
				break;
			case COMPRESSOR_XZ:
				strcat(fparams.opt.file_out, ".xz");
				break;
			case COMPRESSOR_LZ4:
				strcat(fparams.opt.file_out, ".lz4");
				break;
			default:
				;
		}
		if (fparams.opt.enc_algorithm){
			strcat(fparams.opt.file_out, ".crypt");
		}
		free(backupdir);
	}

	/* create temporary files */
	if (temp_file(file_hashes) != 0 || temp_file(file_tar) != 0 || temp_file(file_final) != 0){
		fprintf(stderr, "Could not create temporary file.\n");
		return 1;
	}

	fp_hashes = fopen(file_hashes, "wb");
	if (!fp_hashes){
		fprintf(stderr, "Could not open temporary file.\n");
		return 1;
	}

	printf("Adding files to %s...\n", fparams.opt.file_out);
	/* creating the initial tarball */
	fparams.fp_hashes = fp_hashes;
	fparams.tp = tar_create(file_tar, COMPRESSOR_NONE);

	/* enumerate over each directory with fun() */
	for (i = 0; i < fparams.opt.directories_len; ++i){
		enum_files(fparams.opt.directories[i], fun, &fparams, error, NULL);
	}
	fclose(fparams.fp_hashes);
	tar_close(fparams.tp);

	/* creating the files + hashes tarball */
	tar_final = tar_create(file_final, fparams.opt.comp_algorithm);
	tar_add_file_ex(tar_final, file_tar, "/files", 1, "Compressing files...");
	tar_add_file_ex(tar_final, file_hashes, "/checksums", 1, "Adding checksums...");
	tar_close(tar_final);

	/* getting the final tarball to its destination */
	fp_final = fopen(file_final, "rb");
	fp_out = fopen(fparams.opt.file_out, "wb");
	if (!fp_final || !fp_out){
		fprintf(stderr, "Could not open output file for writing.\n");
		return 1;
	}

	/* encrypt output */
	if (fparams.opt.enc_algorithm){
		char pwbuffer[1024];
		char pwbuffer2[sizeof(pwbuffer)];
		unsigned long err = 0;

		/* disable core dumps if possible */
		if (disable_core_dumps() != 0){
			fprintf(stderr, "WARNING: Core dumps could not be disabled\n");
		}

		crypt_set_encryption(fparams.opt.enc_algorithm, &fk);
		crypt_set_salt((unsigned char*)"\0\0\0\0\0\0\0", &fk);

		/* PASSWORD IN MEMORY */
		printf("Enter encryption password: ");
		get_pass(pwbuffer, sizeof(pwbuffer) - 30);
		printf("\n");
		printf("Verify encryption password: ");
		get_pass(pwbuffer2, sizeof(pwbuffer2) - 30);
		printf("\n");
		while (secure_memcmp(pwbuffer, pwbuffer2, strlen(pwbuffer)) != 0){
			printf("Passwords do not match\n");
			printf("Enter encryption password: ");
			get_pass(pwbuffer, sizeof(pwbuffer) - 30);
			printf("\n");
			printf("Verify encryption password: ");
			get_pass(pwbuffer2, sizeof(pwbuffer2) - 30);
			printf("\n");
		}
		crypt_scrub((unsigned char*)pwbuffer2, strlen(pwbuffer2) + 10 + crypt_randc() % 21);

		if ((err = crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, &fk)) != 0){
			crypt_scrub((unsigned char*)pwbuffer, strlen(pwbuffer) + 10 + crypt_randc() % 21);
			printf("%s\n", err_strerror(err));
			return 1;
		}
		/* don't need to scrub entire buffer, just where the password was
		 * and a little more so attackers don't know how long the password was */
		crypt_scrub((unsigned char*)pwbuffer, strlen(pwbuffer) + 10 + crypt_randc() % 21);
		/* PASSWORD OUT OF MEMORY */

		if ((err = crypt_encrypt(file_final, &fk, fparams.opt.file_out)) != 0){
			printf("%s\n", err_strerror(err));
			return 1;
		}
		/* shreds keys as well */
		crypt_free(&fk);
	}
	else{
		/* mv fp_final to fp_out */
		if (rename(file_final, fparams.opt.file_out) < 0){
			/* file_tar and file_out are on different disks
			 * have to copy to file_out and remove file_tar */
			unsigned char buffer[BUFFER_LEN];
			int len;

			fp_final = fopen(file_final, "rb");
			if (!fp_final){
				fprintf(stderr, "Could not open temporary file.\n");
				return 1;
			}

			while ((len = read_file(fp_final, buffer, sizeof(buffer))) > 0){
				fwrite(buffer, 1, len, fp_out);
			}
			if (ferror(fp_out)){
				fprintf(stderr, "Error writing final output.\n");
			}
		}
	}
	fclose(fp_final);
	fclose(fp_out);

	free_options(&(fparams.opt));
	return 0;
}
