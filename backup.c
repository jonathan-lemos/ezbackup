/* backup.c -- backup backend
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "backup.h"
#include "filehelper.h"
#include "crypt/crypt_easy.h"
#include "fileiterator.h"
#include "error.h"
#include "checksum.h"
#include "options.h"
#include "stringhelper.h"
#include <string.h>
#include <sys/resource.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>

#define UNUSED(x) ((void)x)

struct backup_params{
	TAR*                  tp;
	FILE*                 fp_hashes;
	FILE*                 fp_hashes_prev;
	const struct options* opt;
};

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
		if (bparams->opt->flags.bits.flag_verbose){
			printf("Skipping unchanged (%s)\n", file);
		}
		return 1;
	}
	else if (err != 0){
		log_debug("add_checksum_to_file() failed");
	}

	if (tar_add_file_ex(bparams->tp, file, file, bparams->opt->flags.bits.flag_verbose, file) != 0){
		log_debug("Failed to add file to tar");
	}

	return 1;
}

int error(const char* file, int __errno, void* params){
	UNUSED(params);
	fprintf(stderr, "%s: %s\n", file, strerror(__errno));

	return 1;
}

const char* get_archive_extension(void){
	return ".tar";
}

const char* get_compression_extension(enum COMPRESSOR comp){
	switch (comp){
		case COMPRESSOR_GZIP:
			return ".gz";
		case COMPRESSOR_BZIP2:
			return ".bz2";
		case COMPRESSOR_XZ:
			return ".xz";
		case COMPRESSOR_LZ4:
			return ".lz4";
		case COMPRESSOR_NONE:
			return "";
		default:
			log_einval(comp);
			return NULL;
	}
}

char* get_default_backup_base(const char* dir){
	char file[64];
	char* ret;

	ret = sh_dup(dir);
	if (!ret){
		log_enomem();
		return NULL;
	}
	if (ret[strlen(ret) - 1] == '/'){
		ret[strlen(ret) - 1] = '\0';
	}

	sprintf(file, "/backup-%ld", (long)time(NULL));
	ret = sh_concat(ret, file);
	return ret;
}

char* add_backup_extension(char* base, const struct options* opt){
	base = sh_concat(base, get_archive_extension());
	if (opt->comp_algorithm != COMPRESSOR_NONE){
		base = sh_concat(base, get_compression_extension(opt->comp_algorithm));
	}
	if (opt->enc_algorithm){
		base = sh_concat(base, ".");
		base = sh_concat(base, EVP_CIPHER_name(opt->enc_algorithm));
	}

	return base;
}

FILE* extract_prev_checksums(const char* in, char* _template){
	FILE* fp = NULL;

	fp = temp_fopen(_template);
	if (!fp){
		log_efopen(_template);
		goto cleanup;
	}
	if (fclose(fp) != 0){
		log_efclose(in);
	}

	if (tar_extract_file(in, "/checksums", _template) != 0){
		fp = NULL;
		goto cleanup;
	}

	fp = fopen(_template, "rb");
	if (!fp){
		log_efopen(_template);
	}

cleanup:
	return fp;
}

int add_auxillary_files(TAR* in, const char* hashes, const char* hashes_prev, const struct options* opt, int verbose){
	char template_removed[] = "/var/tmp/removed_XXXXXX";
	char template_config[] = "/var/tmp/config_XXXXXX";
	FILE* fp_removed = NULL;
	FILE* fp_config = NULL;
	int ret = 0;

	if (tar_add_file_ex(in, hashes, "/checksums", verbose, "Adding checksum list...") != 0){
		log_warning("Failed to add checksum list");
		ret = -1;
	}

	if (hashes_prev){
		fp_removed = temp_fopen(template_removed);
		if (!fp_removed){
			log_efopen(template_removed);
			ret = -1;
		}
		fclose(fp_removed);
		fp_removed = NULL;

		if (create_removed_list(hashes_prev, template_removed) != 0){
			log_warning("Failed to create removed list");
			ret = -1;
		}

		if (tar_add_file_ex(in, template_removed, "/removed", verbose, "Adding removed list...") != 0){
			log_warning("Failed to add removed list");
			ret = -1;
		}
	}

	fp_config = temp_fopen(template_config);
	if (!fp_config){
		log_efopen(template_config);
		ret = -1;
	}
	fclose(fp_config);
	fp_config = NULL;

	if (write_options_tofile(template_config, opt) != 0){
		log_warning("Failed to write current config to file");
		ret = -1;
	}

	if (tar_add_file_ex(in, template_config, "/config", verbose, "Adding config file...") != 0){
		log_warning("Failed to add config list");
		ret = -1;
	}

	fp_removed ? fclose(fp_removed) : 0;
	fp_config ? fclose(fp_config) : 0;
	remove(template_removed);
	remove(template_config);

	return ret;
}

int backup(const struct options* opt){
	struct backup_params bp;
	TAR* tp_final = NULL;
	char template_tar_files[] = "/var/tmp/tar_XXXXXX";
	char template_tar_complete[] = "/var/tmp/complete_XXXXXX";
	char template_hashes[] = "/var/tmp/hashes_XXXXXX";
	char template_hashes_prev[] = "/var/tmp/prev_XXXXXX";
	FILE* fp_tar_files = NULL;
	FILE* fp_tar_complete = NULL;
	char* file_out = NULL;
	char* backup_intar = NULL;

	int ret = 0;
	size_t i;

	/* load bp.opt */
	memset(&bp, 0, sizeof(bp));
	bp.opt = opt;

	/* create temp files */
	fp_tar_files = temp_fopen(template_tar_files);
	fp_tar_complete = temp_fopen(template_tar_complete);
	bp.fp_hashes = temp_fopen(template_hashes);
	if (!fp_tar_files ||
			!fp_tar_complete ||
			!bp.fp_hashes){
		log_debug("Failed to create temp file");
		ret = -1;
		goto cleanup;
	}
	fclose(fp_tar_files);
	fp_tar_files = NULL;
	fclose(fp_tar_complete);
	fp_tar_complete = NULL;

	if (opt->prev_backup){
		if ((bp.fp_hashes_prev = extract_prev_checksums(opt->prev_backup, template_hashes_prev)) == NULL){
			log_debug("Failed to extract prev checksums");
			ret = -1;
			goto cleanup;
		}
	}

	/* prepare output paths */
	file_out = sh_concat(get_default_backup_base(opt->output_directory), get_archive_extension());
	backup_intar = add_backup_extension(sh_dup("/files"), opt);
	if (!file_out || !backup_intar){
		log_error("Failed to determine backup name");
		ret = -1;
		goto cleanup;
	}

	/* create tar */
	printf("Adding files to %s...\n", file_out);
	bp.tp = tar_create(template_tar_files, bp.opt->comp_algorithm, bp.opt->comp_level);
	if (!bp.tp){
		log_debug("Failed to create tar");
		ret = -1;
		goto cleanup;
	}

	/* add files to tar */
	for (i = 0; i < bp.opt->directories->len; ++i){
		enum_files(bp.opt->directories->strings[i], func, &bp, error, NULL);
	}
	if (fclose(bp.fp_hashes) != 0){
		log_efclose(template_hashes);
	}
	bp.fp_hashes = NULL;
	if (bp.fp_hashes_prev && fclose(bp.fp_hashes_prev) != 0){
		log_efclose(template_hashes_prev);
	}
	bp.fp_hashes_prev = NULL;
	if (tar_close(bp.tp) != 0){
		log_warning("Failed to close tar. Data corruption possible");
	}
	bp.tp = NULL;

	/* create final tar */
	tp_final = tar_create(file_out, COMPRESSOR_NONE, 0);
	if (!tp_final){
		log_error("Failed to create final tar");
		ret = -1;
		goto cleanup;
	}

	if (tar_add_file_ex(tp_final, template_tar_files, backup_intar, bp.opt->flags.bits.flag_verbose, file_out)){
		log_error("Failed to add files to final output");
		ret = -1;
		goto cleanup;
	}

	/* add /checksums /config /removed */
	if (add_auxillary_files(tp_final, template_hashes, opt->prev_backup ? template_hashes_prev : NULL, bp.opt, bp.opt->flags.bits.flag_verbose) != 0){
		log_debug("Failed to add one or more auxillary files");
	}

	if (tar_close(tp_final) != 0){
		log_warning("Failed to close final output. Data corruption possible");
	}

	/* encrypt output */
	if (bp.opt->enc_algorithm){
		if (easy_encrypt_inplace(file_out, bp.opt->enc_algorithm, bp.opt->flags.bits.flag_verbose) != 0){
			log_warning("Failed to encrypt file");
		}
	}

	/* previous backup is now current backup */
	set_home_conf_dir(file_out);

cleanup:
	if (fp_tar_files && fclose(fp_tar_files) != 0){
		log_efclose(template_tar_files);
	}
	if (fp_tar_complete && fclose(fp_tar_complete) != 0){
		log_efclose(template_tar_complete);
	}
	if (bp.tp && tar_close(bp.tp) != 0){
		log_warning("Failed to close tar. Data corruption possible");
	}
	if (tp_final && tar_close(tp_final) != 0){
		log_warning("Failed to close final tar. Data corruption possible");
	}
	remove(template_tar_files);
	remove(template_tar_complete);
	remove(template_hashes);
	remove(template_hashes_prev);
	free(file_out);
	return ret;
}
