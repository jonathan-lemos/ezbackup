/** @file backup.c
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
#include "cloud/base.h"
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#define UNUSED(x) ((void)x)

static int make_internal_directory_paths(const char* dir, char** dir_files, char** dir_deltas){
	int ret = 0;
	if (dir_files){
		*dir_files = sh_concat_path(sh_dup(dir), "/files");
		if (!(*dir_files)){
			log_error("Failed to create dir_files.");
			ret = -1;
			goto cleanup;
		}
	}
	if (dir_deltas){
		*dir_deltas = sh_concat_path(sh_dup(dir), "/deltas");
		if (!(*dir_deltas)){
			log_error("Failed to create dir_deltas.");
			ret = -1;
			goto cleanup;
		}
	}
cleanup:
	if (ret != 0){
		dir_files ? free(*dir_files) : (void)0;
		dir_deltas ? free(*dir_deltas) : (void)0;
	}
	return 0;
}

static int make_file_paths(const char* file, const char* base_directory, unsigned long backup_time, char** out_file_path, char** out_delta_path){
	char* output_files = NULL;
	char* output_deltas = NULL;
	int ret = 0;

	if (out_file_path || out_delta_path){
		if (!base_directory){
			log_warning("output_directory is NULL when it is needed to determine out_file_path or out_delta_path.");
			ret = -1;
			goto cleanup;
		}
		if (make_internal_directory_paths(base_directory, &output_files, &output_deltas) != 0){
			log_error("Failed to determine internal directory paths.");
			ret = -1;
			goto cleanup;
		}
	}

	if (out_file_path){
		*out_file_path = sh_concat_path(sh_dup(output_files), file);
		if (!(*out_file_path)){
			log_error("Failed to create out_file_path.");
			ret = -1;
			goto cleanup;
		}
	}
	if (out_delta_path){
		char tmp[16];
		sprintf(tmp, "%lu", backup_time);
		*out_delta_path = sh_concat_path(sh_dup(output_deltas), file);
		*out_delta_path = sh_concat(*out_delta_path, tmp);
		if (!(*out_delta_path)){
			log_error("Failed to create out_delta_path.");
			ret = -1;
			goto cleanup;
		}
	}

cleanup:
	if (ret != 0){
		out_file_path ? free(*out_file_path) : (void)0;
		out_delta_path ? free(*out_delta_path) : (void)0;
	}
	free(output_files);
	free(output_deltas);
	return ret;
}

static int cloud_copy_single_file(const char* file, const char* cloud_directory, struct cloud_data* cd, unsigned backup_time){
	char* cloud_path_files = NULL;
	char* cloud_path_delta = NULL;
	int ret = 0;

	if (make_file_paths(file, cloud_directory, backup_time, &cloud_path_files, &cloud_path_delta) != 0){
		log_error("Failed to create cloud paths.");
		ret = -1;
		goto cleanup;
	}

	if (cloud_stat(cloud_path_files, NULL, cd) == 0 && cloud_rename(cloud_path_files, cloud_path_delta, cd) != 0){
		log_warning_ex("Failed to create delta for %s.", cloud_path_files);
	}

	if (cloud_upload(file, cloud_path_files, cd) != 0){
		log_error_ex("Failed to upload %s to the cloud.", file);
		ret = -1;
		goto cleanup;
	}

cleanup:
	free(cloud_path_files);
	free(cloud_path_delta);
	return ret;
}

static int copy_single_file(const char* file, const struct options* opt, union zip_flags c_flags, unsigned long backup_time, struct cloud_data* cd, const char* cloud_directory){
	char* path_files = NULL;
	char* path_delta = NULL;
	char* file_parent = NULL;
	char* delta_parent = NULL;
	int ret = 0;

	if (make_file_paths(file, opt->output_directory, backup_time, &path_files, &path_delta) != 0){
		log_error("Failed determining file path or delta path");
		ret = -1;
		goto cleanup;
	}

	file_parent = sh_parent_dir(path_files);
	delta_parent = sh_parent_dir(path_delta);
	if (mkdir_recursive(file_parent) < 0 || mkdir_recursive(delta_parent) < 0){
		log_warning("Failed to make one or more parent directories.");
	}

	if (file_exists(path_files) && rename_file(path_files, path_delta) != 0){
		log_warning_ex("Failed to create delta for %s", path_files);
	}

	if (zip_compress(file, path_files, opt->c_type, opt->c_level, c_flags) != 0){
		log_error("Failed to compress output file");
		ret = -1;
		goto cleanup;
	}

	if (opt->enc_algorithm && easy_encrypt_inplace(path_files, EVP_CIPHER_name(opt->enc_algorithm), opt->flags.bits.flag_verbose, opt->enc_password) != 0){
		log_error("Failed to encrypt file");
		ret = -1;
		goto cleanup;
	}

	if (cd && cloud_copy_single_file(path_files, cloud_directory, cd, backup_time) != 0){
		log_warning_ex("Failed to upload %s to the cloud", path_files);
		ret = -1;
	}

cleanup:
	free(path_files);
	free(path_delta);
	free(file_parent);
	free(delta_parent);
	return ret;
}

static int copy_files(const struct options* opt, FILE* fp_checksum, FILE* fp_checksum_prev, unsigned long backup_time){
	char* password = NULL;
	struct cloud_data* cd = NULL;
	int ret = 0;
	size_t i;

	if (opt->cloud_options->cp != CLOUD_NONE && cloud_login(opt->cloud_options, &cd) != 0){
		log_error("Could not connect to the cloud.");
		ret = -1;
		goto cleanup;
	}

	if (opt->enc_algorithm && !opt->enc_password){
		int res;

		while ((res = crypt_getpassword("Enter  encryption password:", "Verify encryption password:", &password)) > 0);

		if (res < 0){
			log_error("Failed to read encryption password from terminal");
			ret = -1;
			goto cleanup;
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
				printf("%s\n", tmp);
				if (copy_single_file(tmp, opt, opt->c_flags, backup_time, cd, opt->cloud_options->upload_directory) != 0){
					log_warning_ex("Failed to copy %s", tmp);
				}
			}
			else{
				log_error_ex("Failed to calculate checksum for %s", tmp);
			}

			free(tmp);
		}
		fi_end(fis);
	}

cleanup:
	cloud_logout(cd);
	free(password);
	return ret;
}

int backup(const struct options* opt){
	char* dir_files = NULL;
	char* dir_deltas = NULL;
	char* checksum_path = NULL;
	char* checksum_prev_path = NULL;
	FILE* fp_checksum = NULL;
	FILE* fp_checksum_prev = NULL;
	char buf[64];
	unsigned long backup_time = time(NULL);
	int ret = 0;

	if (mkdir_recursive(opt->output_directory) < 0){
		log_error("Failed to create output directory");
		ret = -1;
		goto cleanup;
	}

	checksum_path = sh_concat_path(sh_dup(opt->output_directory), "checksums.txt");
	sprintf(buf, ".%lu", backup_time);
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

	if (copy_files(opt, fp_checksum, fp_checksum_prev, backup_time) != 0){
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
