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
#include "crypt/crypt_getpassword.h"
#include "fileiterator.h"
#include "log.h"
#include "checksum.h"
#include "options/options.h"
#include "strings/stringhelper.h"
#include "strings/stringarray.h"
#include "compression/zip.h"
#include <errno.h>
#include <string.h>
#include <time.h>

#define UNUSED(x) ((void)x)

static int create_internal_directories(const char* output_dir, char** dir_files, char** dir_deltas){
	int ret = 0;

	return_ifnull(dir_files, -1);
	return_ifnull(dir_deltas, -1);

	*dir_files = sh_concat_path(sh_dup(output_dir), "/files");
	*dir_deltas = sh_concat_path(sh_dup(output_dir), "/deltas");

	if (!(*dir_files) || !(*dir_deltas)){
		log_error("Failed determining directories for output");
		ret = -1;
		goto cleanup_freeout;
	}

	if (!directory_exists((*dir_files)) && mkdir((*dir_files), 0755) != 0){
		log_error_ex2("Failed to create directory %s (%s)", (*dir_files), strerror(errno));
		ret = -1;
		goto cleanup_freeout;
	}

	if (!directory_exists((*dir_deltas)) && mkdir((*dir_deltas), 0755) != 0){
		log_error_ex2("Failed to create directory %s (%s)", (*dir_deltas), strerror(errno));
		ret = -1;
		goto cleanup_freeout;
	}

	return ret;

cleanup_freeout:
	free(*dir_files);
	*dir_files = NULL;
	free(*dir_deltas);
	*dir_deltas = NULL;
	return ret;
}

static int mkdir_recursive(const char* dir){
	struct string_array* components = NULL;
	size_t i;

	components = sa_get_parent_dirs(dir);
	if (!components){
		log_error("Failed to get parent dirs");
		return -1;
	}

	for (i = 0; i < components->len; ++i){
		if (!directory_exists(components->strings[i]) && mkdir(components->strings[i], 0755) != 0){
			log_error_ex2("Failed to make directory %s (%s)", components->strings[i], strerror(errno));
			sa_free(components);
			return -1;
		}
	}

	sa_free(components);
	return 0;
}

static int make_internal_subdirectories(const char* file_dir, const char* delta_dir, const struct string_array* directories, const struct string_array* exclude){
	size_t i;
	for (i = 0; i < directories->len; ++i){
		struct fi_stack* fis;
		char* tmp;
		size_t j;

		fis = fi_start(directories->strings[i]);
		if (!fis){
			log_warning_ex("Failed to fi_start in directory %s", directories->strings[i]);
		}

		while ((tmp = fi_next(fis)) != NULL){
			char* file_subdir = sh_concat_path(sh_dup(file_dir), fi_directory_name(fis));
			char* delta_subdir = sh_concat_path(sh_dup(delta_dir), fi_directory_name(fis));

			free(tmp);

			for (j = 0; j < exclude->len; ++j){
				if (sh_starts_with(fi_directory_name(fis), exclude->strings[j])){
					fi_skip_current_dir(fis);
					break;
				}
			}
			if (j != exclude->len){
				free(file_subdir);
				free(delta_subdir);
				continue;
			}

			if (!file_subdir || !delta_subdir){
				log_warning("Could not determine the path for one or more subdirectories");
				continue;
			}

			if (mkdir_recursive(file_subdir) != 0){
				log_warning_ex("Failed to create file subdirectory at %s", file_subdir);
			}

			if (mkdir_recursive(delta_subdir) != 0){
				log_warning_ex("Failed to create delta subdirectory at %s", delta_subdir);
			}

			free(file_subdir);
			free(delta_subdir);
		}
		fi_end(fis);
	}
	return 0;
}

static int copy_single_file(const char* file, const char* file_dir, const char* delta_dir, enum COMPRESSOR c_type, int compression_level, unsigned c_flags, const EVP_CIPHER* enc_algorithm, const char* enc_password, int verbose){
	char* path_files = NULL;
	char* path_delta = NULL;
	char buf[64];
	int ret = 0;

	path_files = sh_concat_path(sh_dup(file_dir), file);

	sprintf(buf, ".%ld", (long)time(0));
	path_delta = sh_concat(sh_concat_path(sh_dup(delta_dir), file), buf);

	if (!path_files || !path_delta){
		log_error("Failed determining file path or delta path");
		ret = -1;
		goto cleanup;
	}

	if (file_exists(path_files) && rename_file(path_files, path_delta) != 0){
		log_warning_ex("Failed to create delta for %s", path_files);
	}

	if (zip_compress(file, path_files, c_type, compression_level, c_flags) != 0){
		log_error("Failed to compress output file");
		ret = -1;
		goto cleanup;
	}

	if (enc_algorithm && easy_encrypt_inplace(path_files, enc_algorithm, verbose, enc_password) != 0){
		log_error("Failed to encrypt file");
		ret = -1;
		goto cleanup;
	}

cleanup:
	free(path_files);
	free(path_delta);
	return ret;
}

static int copy_files(const struct options* opt, const char* dir_files, const char* dir_deltas, FILE* fp_checksum, FILE* fp_checksum_prev){
	char* password = NULL;
	size_t i;

	if (opt->enc_algorithm && !opt->enc_password){
		int res;

		while ((res = crypt_getpassword("Enter  encryption password:", "Verify encryption password:", &password)) > 0);

		if (res < 0){
			log_error("Failed to read encryption password from terminal");
			return -1;
		}
	}

	for (i = 0; i < opt->directories->len; ++i){
		struct fi_stack* fis = NULL;
		char* tmp;
		int res;

		fis = fi_start(opt->directories->strings[i]);
		if (!fis){
			log_warning_ex("Failed to fi_start in directory %s", opt->directories->strings[i]);
		}
		while ((tmp = fi_next(fis)) != NULL){
			size_t j;
			for (j = 0; j < opt->exclude->len; ++j){
				if (sh_starts_with(tmp, opt->exclude->strings[j])){
					fi_skip_current_dir(fis);
					break;
				}
			}
			if (j != opt->exclude->len){
				free(tmp);
				continue;
			}

			res = add_checksum_to_file(tmp, opt->hash_algorithm, fp_checksum, fp_checksum_prev);
			if (res > 0){
				log_info_ex("File %s was unchanged", tmp);
			}
			else if (res == 0){
				if (copy_single_file(tmp, dir_files, dir_deltas, opt->comp_algorithm, opt->comp_level, 0, opt->enc_algorithm, password ? password : opt->enc_password, opt->flags.bits.flag_verbose) != 0){
					log_warning_ex2("Failed to copy %s to %s", tmp, dir_files);
				}
				else{
					printf("%s\n", tmp);
				}
			}
			else{
				log_error_ex("Failed to calculate checksum for %s", tmp);
			}

			free(tmp);
		}
		fi_end(fis);
	}

	free(password);
	return 0;
}

int backup(const struct options* opt){
	char* dir_files = NULL;
	char* dir_deltas = NULL;
	char* checksum_path = NULL;
	char* checksum_prev_path = NULL;
	FILE* fp_checksum = NULL;
	FILE* fp_checksum_prev = NULL;
	char buf[64];
	int ret = 0;

	if (mkdir_recursive(opt->output_directory) != 0){
		log_error("Failed to create output directory");
		ret = -1;
		goto cleanup;
	}

	if (create_internal_directories(opt->output_directory, &dir_files, &dir_deltas) != 0){
		log_error("Failed to create internal directories");
		ret = -1;
		goto cleanup;
	}

	checksum_path = sh_concat_path(sh_dup(opt->output_directory), "checksums.txt");
	sprintf(buf, ".%ld", (long)time(0));
	checksum_prev_path = sh_concat(sh_dup(checksum_path), buf);

	if (!checksum_path || !checksum_prev_path){
		log_error("Could not determine checksum locations");
		ret = -1;
		goto cleanup;
	}

	if (file_exists(checksum_path) && rename_file(checksum_path, checksum_prev_path) != 0){
		log_warning_ex("Failed to backup old checksum file to %s", checksum_prev_path);
	}

	fp_checksum = fopen(checksum_path, "wb");
	if (!fp_checksum){
		log_efopen(checksum_path);
		ret = -1;
		goto cleanup;
	}

	fp_checksum_prev = fopen(checksum_prev_path, "rb");
	if (!fp_checksum_prev){
		if (errno == ENOENT){
			log_info("Previous checksum file does not exist");
		}
		else{
			log_efopen(checksum_prev_path);
			ret = -1;
			goto cleanup;
		}
	}

	if (make_internal_subdirectories(dir_files, dir_deltas, opt->directories, opt->exclude) != 0){
		log_error("Failed to make internal subdirectories");
		ret = -1;
		goto cleanup;
	}

	if (copy_files(opt, dir_files, dir_deltas, fp_checksum, fp_checksum_prev) != 0){
		log_error("Error copying files to their destinations");
		ret = -1;
		goto cleanup;
	}

	fclose(fp_checksum);
	fp_checksum = NULL;

	if (sort_checksum_file(checksum_path) != 0){
		log_warning("Failed to sort checksum file");
	}

cleanup:
	fp_checksum ? fclose(fp_checksum) : 0;
	fp_checksum_prev ? fclose(fp_checksum_prev) : 0;
	free(dir_files);
	free(dir_deltas);
	free(checksum_path);
	free(checksum_prev_path);
	return ret;
}
