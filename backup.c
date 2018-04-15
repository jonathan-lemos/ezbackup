/* backup.c -- backup backend
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "backup.h"
#include "readfile.h"
#include "crypt.h"
#include "fileiterator.h"
#include "error.h"
#include "checksum.h"
#include "options.h"
#include <string.h>
#include <sys/resource.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>

#define UNUSED(x) ((void)x)

struct backup_params{
	TAR*            tp;
	FILE*           fp_hashes;
	FILE*           fp_hashes_prev;
	struct options* opt;
};

int __coredumps(int enable){
	static struct rlimit rl_prev;
	static int previously_disabled = 0;
	struct rlimit rl;
	int ret = 0;
	if (!enable){
		if (getrlimit(RLIMIT_CORE, &rl_prev) != 0){
			log_warning_ex("Failed to get previous core dump limits (%s)", strerror(errno));
			ret = -1;
		}
		rl.rlim_cur = 0;
		rl.rlim_max = 0;
		if (setrlimit(RLIMIT_CORE, &rl) != 0){
			log_warning_ex("Failed to disable core dumps (%s)", strerror(errno));
			ret = -1;
		}
		previously_disabled = 1;
	}
	else{
		if (!previously_disabled){
			return 0;
		}
		if (setrlimit(RLIMIT_CORE, &rl_prev) != 0){
			log_warning_ex("Failed to restore previous core dump limits (%s)", strerror(errno));
			ret = -1;
		}
		previously_disabled = 0;
	}
	return ret;
}

int disable_core_dumps(void){
	return __coredumps(0);
}

int enable_core_dumps(void){
	return __coredumps(1);
}

int extract_prev_checksums(const char* in, const char* out, const EVP_CIPHER* enc_algorithm, int verbose){
	char pwbuffer[1024];
	char template_decrypt[] = "/var/tmp/decrypt_XXXXXX";
	struct crypt_keys* fk;
	FILE* fp_decrypt = NULL;
	char prompt[128];
	int ret = 0;

	if (!in || !out || !enc_algorithm){
		log_enull();
		return -1;
	}

	if (disable_core_dumps() != 0){
		log_debug("Did not disable core dumps");
	}

	if ((fk = crypt_new()) == NULL){
		log_debug("Failed to initialize crypt_keys");
		return -1;
	}

	if (crypt_set_encryption(enc_algorithm, fk) != 0){
		log_debug("crypt_set_encryption() failed");
		ret = -1;
		goto cleanup;
	}

	if (crypt_extract_salt(in, fk) != 0){
		log_debug("crypt_extract_salt() failed");
		ret = -1;
		goto cleanup;
	}

	sprintf(prompt, "Enter %s decryption password", EVP_CIPHER_name(enc_algorithm));
	if ((crypt_getpassword(prompt, NULL, pwbuffer, sizeof(pwbuffer))) != 0){
		log_debug("crypt_getpassword() failed");
		ret = -1;
		goto cleanup;
	}

	if ((crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, fk)) != 0){
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		log_debug("crypt_gen_keys() failed)");
		ret = -1;
		goto cleanup;
	}
	crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);

	if ((fp_decrypt = temp_fopen(template_decrypt)) == NULL){
		log_debug("temp_file() for file_decrypt failed");
		ret = -1;
		goto cleanup;
	}

	if ((crypt_decrypt_ex(in, fk, template_decrypt, verbose, "Decrypting file...")) != 0){
		log_debug("crypt_decrypt() failed");
		ret = -1;
		goto cleanup;
	}

	if (tar_extract_file(template_decrypt, "/checksums", out) != 0){
		log_debug("tar_extract_file() failed");
		ret = -1;
		goto cleanup;
	}

cleanup:
	crypt_free(fk);

	fp_decrypt ? fclose(fp_decrypt) : 0;
	remove(template_decrypt);

	if (enable_core_dumps() != 0){
		log_debug("enable_core_dumps() failed");
	}
	return ret;
}

int encrypt_file(const char* in, const char* out, const EVP_CIPHER* enc_algorithm, int verbose){
	char pwbuffer[1024];
	struct crypt_keys* fk;
	char prompt[128];
	int ret;

	/* disable core dumps if possible */
	if (disable_core_dumps() != 0){
		log_warning("Core dumps could not be disabled\n");
	}

	if ((fk = crypt_new()) == NULL){
		log_debug("Failed to generate new struct crypt_keys");
		return -1;
	}

	if (crypt_set_encryption(enc_algorithm, fk) != 0){
		log_debug("Could not set encryption type");
		ret = -1;
		goto cleanup;
	}

	if (crypt_gen_salt(fk) != 0){
		log_debug("Could not generate salt");
		ret = -1;
		goto cleanup;
	}

	sprintf(prompt, "Enter %s encryption password", EVP_CIPHER_name(enc_algorithm));
	/* PASSWORD IN MEMORY */
	while ((ret = crypt_getpassword(prompt,
					"Verify encryption password",
					pwbuffer,
					sizeof(pwbuffer))) > 0){
		printf("\nPasswords do not match\n");
	}
	if (ret < 0){
		log_debug("crypt_getpassword() failed");
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		ret = -1;
		goto cleanup;
	}

	if ((crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, fk)) != 0){
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		log_debug("crypt_gen_keys() failed");
		ret = -1;
		goto cleanup;
	}
	/* don't need to scrub entire buffer, just where the password was
	 * and a little more so attackers don't know how long the password was */
	crypt_scrub((unsigned char*)pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
	/* PASSWORD OUT OF MEMORY */

	if ((crypt_encrypt_ex(in, fk, out, verbose, "Encrypting file...")) != 0){
		crypt_free(fk);
		log_debug("crypt_encrypt() failed");
		ret = -1;
		goto cleanup;
	}

cleanup:
	/* shreds keys as well */
	crypt_free(fk);
	if (enable_core_dumps() != 0){
		log_debug("enable_core_dumps() failed");
	}
	return 0;
}

int rename_ex(const char* _old, const char* _new){
	FILE* fp_old;
	FILE* fp_new;
	unsigned char buffer[BUFFER_LEN];
	int len;

	if (rename(_old, _new) == 0){
		return 0;
	}

	fp_old = fopen(_old, "rb");
	if (!fp_old){
		log_efopen(_old);
		return -1;
	}

	fp_new = fopen(_new, "wb");
	if (!fp_new){
		log_efopen(_new);
		fclose(fp_old);
		return -1;
	}

	while ((len = read_file(fp_old, buffer, sizeof(buffer))) > 0){
		fwrite(buffer, 1, len, fp_new);
		if (ferror(fp_new)){
			fclose(fp_old);
			fclose(fp_new);
			log_efwrite(_new);
			return -1;
		}
	}

	fclose(fp_old);

	if (fclose(fp_new) != 0){
		log_efclose(_old);
		return -1;
	}

	remove(_old);
	return 0;
}

int func(const char* file, const char* dir, struct stat* st, void* params){
	struct backup_params* bparams = (struct backup_params*)params;
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
	for (i = 0; i < bparams->opt->exclude_len; ++i){
		/* stop iterating through this directory */
		if (!strcmp(dir, bparams->opt->exclude[i])){
			return 0;
		}
	}

	err = add_checksum_to_file(file, bparams->opt->hash_algorithm, bparams->fp_hashes, bparams->fp_hashes_prev);
	if (err == 1){
		if (bparams->opt->flags & FLAG_VERBOSE){
			printf("Skipping unchanged (%s)\n", file);
		}
		return 1;
	}
	else if (err != 0){
		log_debug("add_checksum_to_file() failed");
	}

	path_in_tar = malloc(strlen(file) + sizeof("/files"));
	if (!path_in_tar){
		log_enomem();
		return 0;
	}
	strcpy(path_in_tar, "/files");
	strcat(path_in_tar, file);

	if (tar_add_file_ex(bparams->tp, file, path_in_tar, bparams->opt->flags & FLAG_VERBOSE, file) != 0){
		log_debug("Failed to add file to tar");
	}
	free(path_in_tar);

	return 1;
}

int error(const char* file, int __errno, void* params){
	UNUSED(params);
	fprintf(stderr, "%s: %s\n", file, strerror(__errno));

	return 1;
}

int get_default_backup_name(struct options* opt, char** out){
	char file[128];

	/* /home/<user>/Backups/backup-<unixtime>.tar(.bz2)(.crypt) */
	*out = malloc(strlen(opt->output_directory) + sizeof(file));
	if (!(out)){
		log_enomem();
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

int add_default_directories(struct options* opt){
	struct passwd* pw;
	const char* homedir;

	if (!(homedir = getenv("HOME"))){
		pw = getpwuid(getuid());
		if (!pw){
			log_error("Failed to get home directory");
			return -1;
		}
		homedir = pw->pw_dir;
	}

	if (opt->directories){
		int i;
		for (i = 0; i < opt->directories_len; ++i){
			free(opt->directories[i]);
		}
		free(opt->directories);
	}

	opt->directories_len = 1;
	opt->directories = malloc(sizeof(*(opt->directories)));
	if (!opt->directories){
		log_enomem();
		return -1;
	}
	opt->directories[0] = malloc(strlen(homedir) + 1);
	if (!opt->directories[0]){
		log_enomem();
		free(opt->directories);
		return -1;
	}

	strcpy(opt->directories[0], homedir);
	return 0;
}

static int add_auxillary_files(TAR* tp, const char* file_hashes, const char* file_hashes_prev, const struct options* opt_prev, int verbose){
	char template_sorted[] = "/var/tmp/removed_XXXXXX";
	char template_removed[] = "/var/tmp/sorted_XXXXXX";
	char template_config_prev[] = "/var/tmp/config_XXXXXX";
	FILE* fp_removed = NULL;
	FILE* fp_sorted = NULL;
	FILE* fp_config_prev = NULL;
	int ret = 0;

	fp_removed = temp_fopen(template_sorted);
	fp_sorted = temp_fopen(template_removed);
	fp_config_prev = temp_fopen(template_config_prev);
	if (!fp_removed ||
			!fp_sorted ||
			!fp_config_prev){
		log_error("Failed to make one or more temporary files");
		ret = -1;
		goto cleanup;
	}
	fclose(fp_removed);
	fclose(fp_sorted);
	fclose(fp_config_prev);

	/* add sorted hashes */
	if (sort_checksum_file(file_hashes, template_sorted) != 0 || fflush(fp_sorted) != 0){
		log_warning("Failed to sort checksum list");
	}
	else if (tar_add_file_ex(tp, template_sorted, "/checksums", verbose, "Adding checksum list...") != 0){
		log_warning("Failed to write checksums to file");
	}

	/* removed file list */
	if (file_hashes_prev){
		if (create_removed_list(file_hashes_prev, template_removed) != 0){
			log_debug("Failed to create removed list");
		}
		else if (tar_add_file_ex(tp, template_removed, "/removed", verbose, "Adding removed list...") != 0){
			log_warning("Failed to add removed list to backup");
		}
	}

	/* add previous config */
	if (opt_prev &&
			(write_config_file(opt_prev, template_config_prev) != 0 ||
			 tar_add_file_ex(tp, template_config_prev, "/config", verbose, "Adding previous config...") != 0)){
		log_warning("Failed to add previous config to file");
	}

cleanup:
	remove(template_sorted);
	remove(template_removed);
	remove(template_config_prev);
	return ret;
}

int backup(struct options* opt, const struct options* opt_prev){
	struct backup_params bp;
	char template_tar[] = "/var/tmp/tar_XXXXXX";
	char template_hashes[] = "/var/tmp/hashes_XXXXXX";
	char template_hashes_prev[] = "/var/tmp/prev_XXXXXX";
	FILE* fp_tar = NULL;
	char* file_out = NULL;
	int ret = 0;
	int i;

	/* load bp.opt */
	memset(&bp, 0, sizeof(bp));
	bp.opt = opt;

	/* create temp files */
	fp_tar = temp_fopen(template_tar);
	bp.fp_hashes = temp_fopen(template_hashes);
	bp.fp_hashes_prev = temp_fopen(template_hashes_prev);
	if (!fp_tar ||
			!(bp.fp_hashes) ||
			!(bp.fp_hashes_prev)){
		log_debug("Failed to create temp file");
		ret = -1;
		goto cleanup;
	}

	if ((!opt->directories || opt->directories_len <= 0) &&
			add_default_directories(opt) != 0){
		log_error("Failed to determine directories");
		return -1;
	}

	/* load bp.tfp_hashes_prev if there's a previous backup */
	if (opt->prev_backup){
		if (extract_prev_checksums(opt->prev_backup, template_hashes_prev, opt_prev->enc_algorithm, opt->flags & FLAG_VERBOSE) != 0){
			log_debug("Failed to extract previous checksums");
			fclose(bp.fp_hashes_prev);
			bp.fp_hashes_prev = NULL;
		}
		else{
			fclose(bp.fp_hashes_prev);
			bp.fp_hashes_prev = fopen(template_hashes_prev, "rb");
			if (!bp.fp_hashes_prev){
				log_efopen(template_hashes_prev);
			}
		}
	}
	else{
		fclose(bp.fp_hashes_prev);
		bp.fp_hashes_prev = NULL;
	}

	/* determine backup name */
	if (get_default_backup_name(opt, &file_out) != 0){
		log_debug("Failed to generate backup name");
		ret = -1;
		goto cleanup;
	}

	/* create tar */
	printf("Adding files to %s...\n", file_out);
	bp.tp = tar_create(template_tar, bp.opt->comp_algorithm, bp.opt->comp_level);
	if (!bp.tp){
		log_debug("Failed to create tar");
		ret = -1;
		goto cleanup;
	}

	/* add files to tar */
	for (i = 0; i < bp.opt->directories_len; ++i){
		enum_files(bp.opt->directories[i], func, &bp, error, NULL);
	}
	if (fclose(bp.fp_hashes) != 0){
		log_efclose(template_hashes);
	}
	bp.fp_hashes = NULL;
	if (bp.fp_hashes_prev && fclose(bp.fp_hashes_prev) != 0){
		log_efclose(template_hashes_prev);
	}
	bp.fp_hashes_prev = NULL;

	/* add /checksums /config /removed */
	if (add_auxillary_files(bp.tp, template_hashes, template_hashes_prev, opt_prev, bp.opt->flags & FLAG_VERBOSE) != 0){
		log_debug("Failed to add one or more auxillary files");
	}

	if (tar_close(bp.tp) != 0){
		log_warning("Failed to close tar. Data corruption possible");
	}
	bp.tp = NULL;

	/* encrypt output */
	if (bp.opt->enc_algorithm){
		if (encrypt_file(template_tar, file_out, bp.opt->enc_algorithm, bp.opt->flags & FLAG_VERBOSE) != 0){
			log_warning("Failed to encrypt file");
		}
	}
	else{
		if (rename_ex(template_tar, file_out) != 0){
			log_warning("Failed to create destination file");
		}
	}

	/* previous backup is now current backup */
	bp.opt->prev_backup = file_out;

cleanup:
	if (fp_tar && fclose(fp_tar) != 0){
		log_efclose(template_tar);
	}
	if (bp.tp && tar_close(bp.tp) != 0){
		log_warning("Failed to close tar. Data corruption possible");
	}
	if (bp.fp_hashes && fclose(bp.fp_hashes) != 0){
		log_efclose(template_hashes);
	}
	if (bp.fp_hashes_prev && fclose(bp.fp_hashes_prev) != 0){
		log_efclose(template_hashes_prev);
	}
	remove(template_tar);
	remove(template_hashes);
	remove(template_hashes_prev);
	return ret;
}
