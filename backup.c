/* backup.c -- backup backend
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "backup.h"
#include "readfile.h"
#include "crypt_easy.h"
#include "fileiterator.h"
#include "error.h"
#include "checksum.h"
#include "options.h"
#include "coredumps.h"
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

int extract_prev_checksums(const char* in, const char* out, const EVP_CIPHER* enc_algorithm, int verbose){
	char template_decrypt[] = "/var/tmp/decrypt_XXXXXX";
	FILE* fp_decrypt = NULL;
	int ret = 0;

	if (!in){
		log_enull(in);
		return -1;
	}
	if (!out){
		log_enull(out);
		return -1;
	}
	if (!enc_algorithm){
		log_enull(enc_algorithm);
		return -1;
	}

	if ((fp_decrypt = temp_fopen(template_decrypt)) == NULL){
		log_debug("Failed to make template_decrypt");
		ret = -1;
		goto cleanup;
	}

	if (easy_decrypt(in, out, enc_algorithm, verbose) != 0){
		log_debug("easy_decrypt() failed");
		ret = -1;
		goto cleanup;
	}

	if (tar_extract_file(template_decrypt, "/checksums", out) != 0){
		log_debug("tar_extract_file() failed");
		ret = -1;
		goto cleanup;
	}

cleanup:
	fp_decrypt ? fclose(fp_decrypt) : 0;
	remove(template_decrypt);
	return ret;
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
			log_efwrite(_new); return -1;
		}
	}

	fclose(fp_old);

	if (fclose(fp_new) != 0){
		log_efclose(_new);
		return -1;
	}

	remove(_old);
	return 0;
}

int func(const char* file, const char* dir, struct stat* st, void* params){
	struct backup_params* bparams = (struct backup_params*)params;
	int err;
	size_t i;
	UNUSED(st);

	/* exclude lost+found */
	if (strlen(dir) > strlen("lost+found") &&
			!strcmp(dir + strlen(dir) - strlen("lost+found"), "lost+found")){
		return 0;
	}
	/* exclude in exclude list */
	for (i = 0; i < bparams->opt->exclude->len; ++i){
		/* stop iterating through this directory */
		if (!strcmp(dir, bparams->opt->exclude->strings[i])){
			return 0;
		}
	}

	err = add_checksum_to_file(file, bparams->opt->hash_algorithm, bparams->fp_hashes, bparams->fp_hashes_prev);
	if (err == 1){
		if (bparams->opt->flag_verbose){
			printf("Skipping unchanged (%s)\n", file);
		}
		return 1;
	}
	else if (err != 0){
		log_debug("add_checksum_to_file() failed");
	}

	if (tar_add_file_ex(bparams->tp, file, file, bparams->opt->flag_verbose, file) != 0){
		log_debug("Failed to add file to tar");
	}

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

int backup(struct options* opt, const char* file_hashes_prev){
	struct backup_params bp;
	char template_tar[] = "/var/tmp/tar_XXXXXX";
	FILE* fp_tar = NULL;
	char* file_out = NULL;
	char* config_out = NULL;
	char* hashes_out = NULL;
	char* removed_out = NULL;
	int ret = 0;
	int i;

	/* load bp.opt */
	memset(&bp, 0, sizeof(bp));
	bp.opt = opt;

	/* create temp files */
	fp_tar = temp_fopen(template_tar);
	if (!fp_tar){
		log_debug("Failed to create temp file");
		ret = -1;
		goto cleanup;
	}

	/* determine backup name */
	if (get_default_backup_name(opt, &file_out) != 0){
		log_debug("Failed to generate backup name");
		ret = -1;
		goto cleanup;
	}

	config_out = malloc(strlen(file_out) + sizeof(".conf"));
	if (!config_out){
		log_enomem();
		ret = -1;
		goto cleanup;
	}
	strcpy(config_out, file_out);
	strcat(config_out, ".conf");

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
		if (easy_encrypt(template_tar, file_out, bp.opt->enc_algorithm, bp.opt->flags & FLAG_VERBOSE) != 0){
			log_warning("Failed to encrypt file");
		}
	}
	else{
		if (rename_ex(template_tar, file_out) != 0){
			log_warning("Failed to create destination file");
		}
	}

	if (write_config_file(opt, config_out) != 0){
		log_warning("Failed to write config file");
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
	free(config_out);
	return ret;
}
