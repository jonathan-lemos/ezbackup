/* backup.c -- backup backend
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "backup.h"
#include "backup_name.h"
#include "filehelper.h"
#include "crypt/crypt_easy.h"
#include "fileiterator.h"
#include "log.h"
#include "checksum.h"
#include "options/options.h"
#include "strings/stringhelper.h"
#include "strings/stringarray.h"
#include "maketar.h"
#include <sys/resource.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>

#define UNUSED(x) ((void)x)

static int exclude_this_dir(const char* dir, const struct string_array* exclude){
	size_t i;
	for (i = 0; i < exclude->len; ++i){
		if (sh_starts_with(dir, exclude->strings[i])){
			return 1;
		}
	}
	return 0;
}

int create_tar_from_directories(const struct options* opt, FILE* fp_hashes, FILE* fp_hashes_prev, const char* out){
	TAR* tp;
	size_t i;
	int ret = 0;

	tp = tar_create(out, opt->comp_algorithm, opt->comp_level);
	if (!tp){
		log_error_ex("Failed to create tar at %s", out);
		ret = -1;
		goto cleanup;
	}
	for (i = 0; i < opt->directories->len; ++i){
		char* buf = NULL;
		if (fi_start(opt->directories->strings[i]) != 0){
			log_warning_ex("Failed to search through directory %s", opt->directories->strings[i]);
			continue;
		}

		while ((buf = fi_get_next()) != NULL){
			int err;
			if (exclude_this_dir(buf, opt->exclude)){
				fi_skip_current_dir();
				free(buf);
				continue;
			}
			err = add_checksum_to_file(buf, opt->hash_algorithm, fp_hashes, fp_hashes_prev);
			if (err > 0){
				log_info_ex("Skipping unchanged (%s)", buf);
				free(buf);
				continue;
			}
			else if (err < 0){
				log_warning_ex("Failed to add %s to checksum file", buf);
			}

			if (tar_add_file_ex(tp, buf, buf, opt->flags.bits.flag_verbose, buf) != 0){
				log_warning_ex("Failed to add %s to tar", buf);
			}

			free(buf);
		}

		fi_end();
	}

cleanup:
	tp ? tar_close(tp) : 0;
	return ret;
}

struct TMPFILE* extract_prev_checksums(const char* in){
	struct TMPFILE* tfp = NULL;

	tfp = temp_fopen();
	if (!tfp){
		log_etmpfopen();
		goto cleanup;
	}

	if (tar_extract_file(in, "/checksums", tfp->name) != 0){
		temp_fclose(tfp);
		tfp = NULL;
		goto cleanup;
	}

	if (temp_fflush(tfp) != 0){
		log_error("Failed to flush temp file");
		temp_fclose(tfp);
		tfp = NULL;
		goto cleanup;
	}

cleanup:
	return tfp;
}

int create_final_tar(const char* tar_out, const char* tar_in, const char* file_hashes, const char* file_hashes_prev, const struct options* opt, int verbose){
	TAR* tp = NULL;
	struct TMPFILE* tfp_removed = NULL;
	struct TMPFILE* tfp_config = NULL;
	char* backup_intar = NULL;
	int ret = 0;

	tp = tar_create(tar_out, COMPRESSOR_NONE, 0);
	if (!tp){
		log_error_ex("Failed to create tar (%s)", tar_out);
		ret = -1;
		goto cleanup;
	}

	backup_intar = get_name_intar(opt);
	if (!backup_intar){
		log_error("Failed to determine backup name");
		ret = -1;
		goto cleanup;
	}

	if (tar_add_file_ex(tp, tar_in, backup_intar, verbose, "Adding files to final output...") != 0){
		log_error("Failed to add files to final output");
		ret = -1;
		goto cleanup;
	}

	if (tar_add_file_ex(tp, file_hashes, "/checksums", verbose, "Adding checksum list...") != 0){
		log_warning("Failed to add checksum list");
		ret = -1;
	}

	if (file_hashes_prev){
		tfp_removed = temp_fopen();
		if (!tfp_removed){
			log_etmpfopen();
			ret = -1;
		}

		if (create_removed_list(file_hashes_prev, tfp_removed->name) != 0){
			log_warning("Failed to create removed list");
			ret = -1;
		}

		if (tar_add_file_ex(tp, tfp_removed->name, "/removed", verbose, "Adding removed list...") != 0){
			log_warning("Failed to add removed list");
			ret = -1;
		}
	}

	tfp_config = temp_fopen();
	if (!tfp_config){
		log_etmpfopen();
		ret = -1;
	}

	if (write_options_tofile(tfp_config->name, opt) != 0){
		log_warning("Failed to write current config to file");
		ret = -1;
	}

	if (tar_add_file_ex(tp, tfp_config->name, "/config", verbose, "Adding config file...") != 0){
		log_warning("Failed to add config list");
		ret = -1;
	}

cleanup:
	free(backup_intar);
	tp ? tar_close(tp) : 0;
	if (tfp_removed){
		temp_fclose(tfp_removed);
	}
	if (tfp_config){
		temp_fclose(tfp_config);
	}

	return ret;
}

int backup(const struct options* opt){
	struct TMPFILE* tfp_tar_files = NULL;
	struct TMPFILE* tfp_hashes = NULL;
	struct TMPFILE* tfp_hashes_prev = NULL;
	char* file_out = NULL;

	int ret = 0;

	/* create temp files */
	tfp_tar_files = temp_fopen();
	tfp_hashes = temp_fopen();
	if (!tfp_tar_files || !tfp_hashes){
		log_debug("Failed to create temp file");
		ret = -1;
		goto cleanup;
	}

	if (opt->prev_backup){
		if ((tfp_hashes_prev = extract_prev_checksums(opt->prev_backup)) == NULL){
			log_debug("Failed to extract prev checksums");
			ret = -1;
			goto cleanup;
		}
	}

	/* prepare output paths */
	file_out = get_output_name(opt->output_directory);
	if (!file_out){
		log_error("Failed to determine backup name");
		ret = -1;
		goto cleanup;
	}

	/* create tar */
	printf("Adding files to %s...\n", file_out);
	if (create_tar_from_directories(opt, tfp_hashes->fp, tfp_hashes_prev ? tfp_hashes_prev->fp : NULL, tfp_tar_files->name) != 0){
		log_error("Failed to create tar");
		ret = -1;
		goto cleanup;
	}

	temp_fflush(tfp_tar_files);

	/* create final tar */
	if (create_final_tar(file_out, tfp_tar_files->name, tfp_hashes->name, tfp_hashes_prev->name, opt, opt->flags.bits.flag_verbose) != 0){
		log_error("Failed to create final tar");
		ret = -1;
		goto cleanup;
	}

	/* encrypt output */
	if (opt->enc_algorithm){
		if (easy_encrypt_inplace(file_out, opt->enc_algorithm, opt->flags.bits.flag_verbose) != 0){
			log_warning("Failed to encrypt file");
		}
	}

	/* previous backup is now current backup */
	set_last_backup_dir(file_out);

cleanup:
	if (tfp_tar_files){
		temp_fclose(tfp_tar_files);
	}
	if (tfp_hashes){
		temp_fclose(tfp_hashes);
	}
	if (tfp_hashes_prev){
		temp_fclose(tfp_hashes_prev);
	}
	free(file_out);
	return ret;
}
