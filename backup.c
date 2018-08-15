/** @file backup.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "backup.h"
#include "cli.h"
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
#include "readline_include.h"
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

static int make_file_paths(const char* file, const char* base_directory, const char* delta_extension, char** out_file_path, char** out_delta_path){
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
		*out_delta_path = sh_concat_path(sh_dup(output_deltas), file);
		*out_delta_path = sh_concat(*out_delta_path, ".");
		*out_delta_path = sh_concat(*out_delta_path, delta_extension);
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

static int cloud_copy_single_file(const char* file_orig_path, const char* file_final, const char* cloud_directory, struct cloud_data* cd, const char* delta_extension){
	char* cloud_path_files = NULL;
	char* cloud_path_delta = NULL;
	char* cloud_parent_files = NULL;
	char* cloud_parent_delta = NULL;
	int ret = 0;

	if (make_file_paths(file_orig_path, cloud_directory, delta_extension, &cloud_path_files, &cloud_path_delta) != 0){
		log_error("Failed to create cloud paths.");
		ret = -1;
		goto cleanup;
	}

	cloud_parent_files = sh_parent_dir(cloud_path_files);
	cloud_parent_delta = sh_parent_dir(cloud_path_delta);
	if (!cloud_parent_files || !cloud_parent_delta){
		log_warning("Failed to create parent directories.");
		ret = -1;
		goto cleanup;
	}

	if (cloud_mkdir(cloud_parent_files, cd) < 0){
		log_warning_ex("Failed to create file parent directory %s.", cloud_parent_files);
		ret = -1;
		goto cleanup;
	}

	if (cloud_stat(cloud_path_files, NULL, cd) == 0){
		if (cloud_mkdir(cloud_parent_delta, cd) < 0){
			log_warning_ex("Failed to create delta parent directory %s.", cloud_parent_delta);
		}
		else if (cloud_rename(cloud_path_files, cloud_path_delta, cd) != 0){
			log_warning_ex("Failed to create delta for %s.", cloud_path_files);
		}
	}

	if (cloud_upload(file_final, cloud_path_files, cd) != 0){
		log_error_ex("Failed to upload %s to the cloud.", file_final);
		ret = -1;
		goto cleanup;
	}

cleanup:
	free(cloud_path_files);
	free(cloud_path_delta);
	free(cloud_parent_files);
	free(cloud_parent_delta);
	return ret;
}

static int copy_single_file(const char* file, const struct options* opt, const char* delta_extension, struct cloud_data* cd, const char* cloud_directory, const char* password){
	char* path_files = NULL;
	char* path_delta = NULL;
	char* file_parent = NULL;
	char* delta_parent = NULL;
	int ret = 0;

	if (make_file_paths(file, opt->output_directory, delta_extension, &path_files, &path_delta) != 0){
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

	if (zip_compress(file, path_files, opt->c_type, opt->c_level, opt->c_flags) != 0){
		log_error("Failed to compress output file");
		ret = -1;
		goto cleanup;
	}

	if (opt->enc_algorithm && easy_encrypt_inplace(path_files, EVP_CIPHER_name(opt->enc_algorithm), opt->flags.bits.flag_verbose, password) != 0){
		log_error("Failed to encrypt file");
		ret = -1;
		goto cleanup;
	}

	if (cd && cloud_copy_single_file(file, path_files, cloud_directory, cd, delta_extension) != 0){
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

static int copy_files(const struct options* opt, const struct cloud_options* co, const char* delta_extension, FILE* fp_checksum, FILE* fp_checksum_prev){
	char* password = NULL;
	struct cloud_data* cd = NULL;
	int ret = 0;
	size_t i;

	if (co->cp != CLOUD_NONE && cloud_login(co, &cd) != 0){
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

			res = add_checksum_to_file(tmp, opt->hash_algorithm, fp_checksum, fp_checksum_prev, NULL);
			if (res > 0){
				log_info_ex("File %s was unchanged", tmp);
			}
			else if (res == 0){
				printf("%s\n", tmp);
				if (copy_single_file(tmp, opt, delta_extension, cd, co->upload_directory, password ? password : opt->enc_password) != 0){
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

static int cloud_remove_deleted_files(const char* checksum_file, const char* delta_extension, const struct cloud_options* co){
	struct TMPFILE* tfp_removed = NULL;
	struct cloud_data* cd = NULL;
	char* tmp;
	int ret = 0;

	if (!file_exists(checksum_file)){
		log_info("Previous checksum file does not exist.");
		return 0;
	}

	tfp_removed = temp_fopen();
	if (!tfp_removed){
		log_warning("Failed to create temporary file.");
		ret = -1;
		goto cleanup;
	}

	if (create_removed_list(checksum_file, tfp_removed->name)){
		log_warning("Failed to create removed list.");
		ret = -1;
		goto cleanup;
	}

	if (temp_fflush(tfp_removed) != 0){
		log_warning("Failed to update temporary file pointer.");
		ret = -1;
		goto cleanup;
	}

	if (cloud_login(co, &cd) != 0){
		log_warning("Failed to log into cloud account.");
		ret = -1;
		goto cleanup;
	}

	while ((tmp = get_next_removed(tfp_removed->fp)) != NULL){
		char* file_path = NULL;
		char* delta_path = NULL;
		char* delta_path_parent = NULL;

		if (make_file_paths(tmp, co->upload_directory, delta_extension, &file_path, &delta_path) != 0){
			log_warning_ex("Failed to create file paths for %s", tmp);
			goto cleanup_inner_loop;
		}

		if ((delta_path_parent = sh_parent_dir(delta_path)) == NULL){
			log_warning_ex("Failed to determine parent dir for %s", delta_path);
			goto cleanup_inner_loop;
		}

		if (cloud_mkdir(delta_path_parent, cd) < 0){
			log_warning_ex("Failed to create directory %s", delta_path);
			goto cleanup_inner_loop;
		}

		if (cloud_rename(file_path, delta_path, cd) != 0){
			log_warning_ex2("Failed to rename %s to %s", file_path, delta_path);
			goto cleanup_inner_loop;
		}

		if (cloud_remove(file_path, cd) != 0){
			log_warning_ex("Failed to remove %s", file_path);
			goto cleanup_inner_loop;
		}

cleanup_inner_loop:
		free(file_path);
		free(delta_path);
		free(delta_path_parent);
		free(tmp);
	}

cleanup:
	temp_fclose(tfp_removed);
	cloud_logout(cd);
	return ret;
}

static int create_checksum_files(const char* checksum_file, const char* delta_extension, FILE** out_checksum, FILE** out_checksum_prev){
	FILE* fp_checksum = NULL;
	FILE* fp_checksum_prev = NULL;
	char* checksum_file_prev = NULL;
	int ret = 0;

	return_ifnull(checksum_file, -1);
	return_ifnull(out_checksum, -1);
	return_ifnull(out_checksum_prev, -1);

	if (!file_exists(checksum_file)){
		fp_checksum = fopen(checksum_file, "wb");
		if (!fp_checksum){
			log_efopen(checksum_file);
			ret = -1;
			goto cleanup;
		}
		*out_checksum = fp_checksum;
		*out_checksum_prev = NULL;
		return 0;
	}

	checksum_file_prev = sh_sprintf("%s.%s", checksum_file, delta_extension);
	if (!checksum_file_prev){
		log_warning("Failed to determine checksum delta location.");
		ret = -1;
		goto cleanup;
	}

	if (rename_file(checksum_file, checksum_file_prev) != 0){
		log_warning("Failed to backup old checksum file.");
		ret = -1;
		goto cleanup;
	}

	fp_checksum = fopen(checksum_file, "wb");
	if (!fp_checksum){
		log_efopen(checksum_file);
		ret = -1;
		goto cleanup;
	}

	fp_checksum_prev = fopen(checksum_file_prev, "rb");
	if (!fp_checksum_prev){
		log_efopen(checksum_file_prev);
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (ret == 0){
		*out_checksum = fp_checksum;
		*out_checksum_prev = fp_checksum_prev;
	}
	else{
		*out_checksum = NULL;
		*out_checksum_prev = NULL;
		fp_checksum ? fclose(fp_checksum) : 0;
		fp_checksum_prev ? fclose(fp_checksum_prev) : 0;
	}
	free(checksum_file_prev);
	return ret;
}

static struct cloud_options* generate_filled_co(const struct cloud_options* co){
	struct cloud_options* ret = co_new();
	if (!ret){
		log_error("Failed to generate new cloud options structure");
		return NULL;
	}

	co_set_cp(ret, co->cp);
	if (ret->cp == CLOUD_NONE){
		return ret;
	}

	if (!co->username){
		char* tmp = readline("Cloud username:");
		if (!tmp){
			log_error("Failed to read username from stdin");
			co_free(ret);
			return NULL;
		}
		if (strlen(tmp) == 0){
			log_info("No username specified.");
			co_set_cp(ret, CLOUD_NONE);
			return ret;
		}
		if (co_set_username(ret, tmp) != 0){
			log_error("Failed to set username in cloud options structure");
			free(tmp);
			co_free(ret);
			return NULL;
		}
		free(tmp);
	}
	else{
		if (co_set_username(ret, co->username) != 0){
			log_error("Failed to set username in cloud options structure");
			co_free(ret);
			return NULL;
		}
	}

	if (!co->password){
		char* tmp;
		int res;
		while ((res = crypt_getpassword("Cloud password:", "Verify password:", &tmp)) > 0);
		if (res < 0){
			log_error("Failed to read password from stdin");
			co_free(ret);
			return NULL;
		}
		if (!tmp || strlen(tmp) == 0){
			log_info("Password not specified.");
			co_set_cp(ret, CLOUD_NONE);
			free(tmp);
			return ret;
		}
		if (co_set_password(ret, tmp) != 0){
			log_error("Failed to set password in cloud options structure");
			free(tmp);
			co_free(ret);
			return NULL;
		}
		free(tmp);
	}

	if (co_set_upload_directory(ret, co->upload_directory) != 0){
		log_error("Failed to set upload directory.");
		co_free(ret);
		return NULL;
	}

	return ret;
}

static int is_cloud_disk_synced(const char* checksum_file, const char* cloud_directory, struct cloud_data* cd){
	char* cloud_path = NULL;
	unsigned char* cloud_hash = NULL;
	unsigned cloud_hash_len = 0;
	unsigned char* disk_hash = NULL;
	unsigned disk_hash_len = 0;
	struct TMPFILE* tfp_cloud = NULL;
	int ret = 1;
	int res;

	tfp_cloud = temp_fopen();
	if (!tfp_cloud){
		log_error("Failed to open temporary file");
		ret = -1;
		goto cleanup;
	}

	cloud_path = sh_concat_path(sh_dup(cloud_directory), "checksums.txt");
	if (!cloud_path){
		log_error("Failed to determine path of cloud checksum file.");
		ret = -1;
		goto cleanup;
	}

	res = cloud_download(cloud_path, &(tfp_cloud->name), cd);
	if (res > 0){
		log_debug("Checksum file does not exist.");
		ret = 0;
		goto cleanup;
	}
	else if (res < 0){
		log_error("Failed to download checksum file.");
		ret = -1;
		goto cleanup;
	}

	if (checksum(tfp_cloud->name, NULL, &cloud_hash, &cloud_hash_len) ||
			checksum(checksum_file, NULL, &disk_hash, &disk_hash_len) ||
			cloud_hash_len != disk_hash_len){
		log_error("Failed to checksum one or more files.");
		ret = -1;
		goto cleanup;
	}

	ret = memcmp(cloud_hash, disk_hash, cloud_hash_len) == 0;

cleanup:
	free(cloud_path);
	free(cloud_hash);
	free(disk_hash);
	temp_fclose(tfp_cloud);
	return ret;
}

int backup_wipe(const char* output_directory, const struct cloud_options* co){
	struct cloud_data* cd = NULL;
	int ret = 0;

	log_info("Wiping backups...");

	if (cloud_login(co, &cd) != 0){
		log_error("Failed to log in to cloud provider");
		ret = -1;
		goto cleanup;
	}

	if (cloud_remove(co->upload_directory, cd) != 0){
		log_warning("Failed to remove cloud directory.");
		ret = -1;
		goto cleanup;
	}

	if (rmdir_recursive(output_directory) != 0){
		log_warning("Failed to remove local directory.");
		ret = -1;
		goto cleanup;
	}

cleanup:
	cloud_logout(cd);
	return ret;
}

int cloud_sync_disk(const char* checksum_file, const struct cloud_options* co){
	const char* dialog_options[] = {
		"1) Cloud",
		"2) Disk",
		"3) Abort"
	};
	struct cloud_data* cd = NULL;
	int ret = 0;
	int res;

	if (cloud_login(co, &cd) != 0){
		log_error("Failed to log in to cloud account");
		ret = -1;
		goto cleanup;
	}

	res = is_cloud_disk_synced(checksum_file, co->upload_directory, cd);
	if (res > 0){
		log_info("Cloud and disk are already synced");
		ret = 0;
		goto cleanup;
	}
	else if (res < 0){
		log_error("Failed to check if cloud is synchronized with disk");
		ret = -1;
		goto cleanup;
	}

	res = display_dialog(dialog_options, sizeof(dialog_options) / sizeof(dialog_options[0]),
			"Cloud and disk are not synced.\n"
			"1) Sync from cloud. This will replace any backups on disk.\n"
			"2) Sync from disk. This will replace any backups in the cloud.\n"
			"3) Abort backup.");

	switch (res){

	}

cleanup:
	cloud_logout(cd);
	return ret;
}

int backup(const struct options* opt){
	char* checksum_path = NULL;
	FILE* fp_checksum = NULL;
	FILE* fp_checksum_prev = NULL;
	struct cloud_options* co_true = NULL;
	unsigned long backup_time = time(NULL);
	char delta_extension[16];
	int ret = 0;

	sprintf(delta_extension, "%lu", backup_time);

	if ((co_true = generate_filled_co(opt->cloud_options)) == NULL){
		log_error("Failed to generate cloud options structure.");
		ret = -1;
		goto cleanup;
	}

	if (mkdir_recursive(opt->output_directory) < 0){
		log_error("Failed to create output directory");
		ret = -1;
		goto cleanup;
	}
	checksum_path = sh_concat_path(sh_dup(opt->output_directory), "checksums.txt");
	if (!checksum_path){
		log_error("Failed to determine location of checksum file.");
		ret = -1;
		goto cleanup;
	}

	if (cloud_remove_deleted_files(checksum_path, delta_extension, co_true) != 0){
		log_warning("Failed to remove deleted files since last backup.");
	}

	if (create_checksum_files(checksum_path, delta_extension, &fp_checksum, &fp_checksum_prev) != 0){
		log_warning("Failed to create checksum delta.");
	}
	if (!fp_checksum){
		log_error("Failed to create checksum file.");
		ret = -1;
		goto cleanup;
	}

	if (copy_files(opt, co_true, delta_extension, fp_checksum, fp_checksum_prev) != 0){
		log_error("Error copying files to their destinations");
		ret = -1;
		goto cleanup;
	}

	if (fclose(fp_checksum) != 0){
		log_efclose(checksum_path);
	}
	fp_checksum = NULL;

	if (sort_checksum_file(checksum_path) != 0){
		log_warning("Failed to sort checksum file");
	}

cleanup:
	fp_checksum ? fclose(fp_checksum) : 0;
	fp_checksum_prev ? fclose(fp_checksum_prev) : 0;
	free(checksum_path);
	co_free(co_true);
	return ret;
}
