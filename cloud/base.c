/** @file cloud/base.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "base.h"
#include "../cli.h"
#include "../crypt/crypt_getpassword.h"
#include "../readline_include.h"
#include "../options/options.h"
#include "../log.h"
#include "../strings/stringhelper.h"
#include "mega.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct cloud_functions{
	int (*login)   (const char* username, const char* password, void** out_handle);
	int (*mkdir)   (const char* directory, void* handle);
	int (*readdir) (const char* directory, char*** output, size_t* output_len, void* handle);
	int (*stat)    (const char* file_path, struct stat* out, void* handle);
	int (*rename)  (const char* old_path, const char* new_path, void* handle);
	int (*download)(const char* download_path, const char* out_path, const char* progress_msg, void* handle);
	int (*upload)  (const char* in_file, const char* upload_path, const char* progress_msg, void* handle);
	int (*remove)  (const char* file_path, void* handle);
	int (*logout)  (void* handle);
};

struct cloud_data{
	void* handle;
	const struct cloud_functions* cf;
	const char* name;
};

static int login_null(const char* username, const char* password, void** out_handle){
	(void)username;
	(void)password;
	*out_handle = NULL;
	return 0;
}
static int mkdir_null(const char* directory, void* handle) {
	(void)directory;
	(void)handle;
	return 0;
}
static int readdir_null(const char* directory, char*** output, size_t* output_len, void* handle){
	(void)directory;
	*output = NULL;
	*output_len = 0;
	(void)handle;
	return 0;
}
static int stat_null(const char* file_path, struct stat* out, void* handle){
	(void)file_path;
	memset(out, 0, sizeof(*out));
	(void)handle;
	return 0;
}
static int rename_null(const char* old_path, const char* new_path, void* handle){
	(void)old_path;
	(void)new_path;
	(void)handle;
	return 0;
}
static int download_null(const char* download_path, const char* out_path, const char* progress_msg, void* handle){
	(void)download_path;
	(void)out_path;
	(void)progress_msg;
	(void)handle;
	return 0;
}
static int upload_null(const char* in_path, const char* upload_path, const char* progress_msg, void* handle){
	(void)in_path;
	(void)upload_path;
	(void)progress_msg;
	(void)handle;
	return 0;
}
static int remove_null(const char* file_path, void* handle){
	(void)file_path;
	(void)handle;
	return 0;
}
static int logout_null(void* handle){
	(void)handle;
	return 0;
}
static const struct cloud_functions CF_NULL = {
	login_null,
	mkdir_null,
	readdir_null,
	stat_null,
	rename_null,
	download_null,
	upload_null,
	remove_null,
	logout_null
};
static const struct cloud_functions CF_MEGA = {
	MEGAlogin,
	MEGAmkdir,
	MEGAreaddir,
	MEGAstat,
	MEGArename,
	MEGAdownload,
	MEGAupload,
	MEGArm,
	MEGAlogout
};
static const struct cloud_functions* cloud_provider_to_cloud_functions(enum cloud_provider cp){
	switch (cp){
	case CLOUD_NONE:
		return &CF_NULL;
	case CLOUD_MEGA:
		return &CF_MEGA;
	default:
		log_error("Invalid CLOUD_PROVIDER specified");
		return NULL;
	}
}

static char* read_username_stdin(void){
	char* user = readline("Cloud username:");
	if (!user){
		log_warning("Failed to read from stdin");
		return NULL;
	}
	if (strlen(user) == 0){
		free(user);
		return NULL;
	}
	return user;
}

static char* read_password_stdin(void){
	char* pw;
	int res;
	while ((res = crypt_getpassword("Cloud password:", "Verify password:", &pw)) > 0){
		printf("The passwords do not match.");
	}

	if (res < 0 || !pw){
		log_warning("Failed to read from stdin");
		return NULL;
	}
	if (strlen(pw) == 0){
		free(pw);
		return NULL;
	}
	return pw;
}

static void free_entries(char** entries, size_t entries_len){
	size_t i;
	if (!entries){
		return;
	}
	for (i = 0; i < entries_len; ++i){
		free(entries[i]);
	}
	free(entries);
}

static char* size_tostring(uint64_t size){
	const char* specifiers[] = {
		"B",
		"KiB",
		"MiB",
		"GiB",
		"TiB"
	};
	int specifier_ptr = 0;
	double d_size = size;
	while (d_size >= 1024 && specifier_ptr <= 4){
		specifier_ptr++;
		d_size /= 1024;
	}
	return sh_sprintf(specifier_ptr == 0 ? "%.0f %s" : "%.2f %s", d_size, specifiers[specifier_ptr]);
}

static int confirm_dialog(char* file, int directory, uint64_t size, void* handle, const struct cloud_functions* cf){
	const char* dialog_buttons[] = {
		"Yes",
		"No"
	};
	char* msg = NULL;
	char time_str[256];
	time_t tm = 0;
	struct stat st;
	int res;

	if (!file){
		return 1;
	}

	if (cf->stat(file, &st, handle) != 0){
		log_warning_ex("%s: Failed to stat %s", file);
	}
	else{
		char* size_str = size_tostring(size);
		tm = st.st_ctime;
		/* 01/01/1970 12:00 PM format*/
		strftime(time_str, sizeof(time_str), "%m/%d/%Y %I:%M %p", localtime(&tm));
		msg = directory ?
			sh_sprintf("Directory: %s\nFull path: %s\nCreation time: %s\nIs this the correct directory?", sh_filename(file), file, time_str) :
			sh_sprintf("File: %s\nFull path: %s\nCreation time: %s\nSize: %s\nIs this the correct file?", sh_filename(file), file, time_str, size_str ? size_str : "Unknown");
		if (!msg){
			log_warning("Failed to create creation time msg");
		}
		free(size_str);
	}

	res = display_dialog(dialog_buttons, sizeof(dialog_buttons) / sizeof(dialog_buttons[0]), msg ? msg : "Details of the file could not be determined.");

	free(msg);
	return res == 0;
}

/* if (directories_only){
 *     [Select current directory]
 *     [Parent directory]
 *     [Exit]
 *     ./dir1
 *     ./dir2
 * }
 * else{
 *     [Parent directory]
 *     [Exit]
 *     ./dir1
 *     ./dir2
 *     file1.txt
 *     file2.txt
 * }
 */
static struct string_array* create_menu_entries(const char* base_dir, char*** entries, size_t* entries_len, void* handle, const struct cloud_functions* cf, int directories_only){
	/* final menu entries output */
	struct string_array* sa_final = sa_new();
	/* final actual directory entries output */
	struct string_array* sa_final_entries = sa_new();
	/* directories only. needs to be sorted independently of rest */
	struct string_array* sa_dir = sa_new();
	/* files only. needs to be sorted independently of rest */
	struct string_array* sa_file = sa_new();
	char* tmp = NULL;
	size_t i;

	if (!sa_final || !sa_final_entries || !sa_dir || !sa_file){
		log_error("Failed to create new string_array");
		sa_free(sa_dir);
		sa_free(sa_file);
		sa_free(sa_final);
		sa_free(sa_final_entries);
		return NULL;
	}

	/* adding beginning special entries - THESE CANNOT BE SORTED */

	if (directories_only){
		sa_add(sa_final, "[Select current directory]");
		sa_add(sa_final_entries, base_dir);
	}
	tmp = sh_parent_dir(base_dir);
	/* if parent directory exists */
	if (tmp){
		sa_add(sa_final, "[Parent directory]");
		sa_add(sa_final_entries, tmp);
	}
	free(tmp);
	sa_add(sa_final, "[Exit]");
	sa_add(sa_final_entries, NULL);

	/* add directories to sa_dir and if (!directories_only){files to sa_file} */
	for (i = 0; i < *entries_len; ++i){
		struct stat st;
		if (cf->stat((*entries)[i], &st, handle) != 0){
			log_warning_ex("Failed to stat %s", (*entries)[i]);
		}

		if (S_ISDIR(st.st_mode)){
			sa_add(sa_dir, (*entries)[i]);
		}
		else if (!directories_only){
			sa_add(sa_file, (*entries)[i]);
		}
	}

	sa_sort(sa_dir);
	sa_sort(sa_file);

	for (i = 0; i < sa_dir->len; ++i){
		/* "/dir1/dir2/dir3" -> "./dir3" */
		char* tmp = sh_concat(sh_dup("./"), sh_filename(sa_dir->strings[i]));
		sa_add(sa_final, tmp);
		free(tmp);
	}
	for (i = 0; i < sa_file->len; ++i){
		/* "/dir1/dir2/file.txt" -> "file.txt" */
		sa_add(sa_final, sh_filename(sa_file->strings[i]));
	}

	/* add full paths to sa_final_entries */
	sa_merge(sa_dir, sa_file);
	sa_merge(sa_final_entries, sa_dir);

	if (sa_final->len != sa_final_entries->len){
		log_error("Failed to merge arrays");
		sa_free(sa_final);
		sa_free(sa_final_entries);
		return NULL;
	}

	free_entries(*entries, *entries_len);
	sa_to_raw_array(sa_final_entries, entries, entries_len);
	return sa_final;
}

int cloud_mkdir(const char* dir, struct cloud_data* cd){
	struct string_array* parent_dirs = sa_get_parent_dirs(dir);
	long i;
	int ret = 0;

	if (!parent_dirs){
		log_error("Failed to create parent directories");
		return -1;
	}

	for (i = (long)(parent_dirs->len - 1); i >= 0; --i){
		if (cd->cf->stat(parent_dirs->strings[i], NULL, cd->handle) == 0){
			break;
		}
	}
	++i;
	for (; i < (long)parent_dirs->len; ++i){
		if (cd->cf->mkdir(parent_dirs->strings[i], cd->handle) != 0){
			log_warning_ex2("%s: Failed to create directory %s", cd->name, parent_dirs->strings[i]);
			ret = -1;
		}
	}
	sa_free(parent_dirs);
	return ret;
}

int cloud_mkdir_ui(const char* base_dir, char** chosen_dir, struct cloud_data* cd){
	char* full_directory = NULL;
	char* subdirectory = NULL;
	char* prompt = sh_dup(base_dir);
	struct string_array* parent_dirs = NULL;
	size_t i;
	int ret = 0;

	if (prompt[strlen(prompt) - 1] != '/'){
		prompt = sh_concat(prompt, "/");
	}
	if (!prompt){
		log_warning("Failed to create prompt");
	}

	printf("Make directory:\n");
	subdirectory = readline(prompt ? prompt : "./");
	if (strlen(subdirectory) == 0){
		log_info("User chose not to create a directory");
		ret = 1;
		goto cleanup;
	}
	full_directory = sh_concat_path(sh_dup(base_dir), subdirectory);

	parent_dirs = sa_get_parent_dirs(full_directory);
	if (!parent_dirs){
		log_error("Failed to create string_array");
		ret = -1;
		goto cleanup;
	}

	for (i = 0; i < parent_dirs->len; ++i){
		if (cd->cf->mkdir(parent_dirs->strings[i], cd->handle) < 0){
			log_warning_ex2("%s: Failed to make parent directory %s", cd->name, parent_dirs->strings[i]);
			ret = -1;
		}
	}

cleanup:
	if (chosen_dir){
		if (ret == 0){
			*chosen_dir = full_directory;
		}
		else{
			free(full_directory);
		}
	}
	else{
		free(full_directory);
	}
	free(subdirectory);
	free(prompt);
	sa_free(parent_dirs);
	return ret;
}

int cloud_stat(const char* dir_or_file, struct stat* out, struct cloud_data* cd){
	struct stat st;
	int res;
	if ((res = cd->cf->stat(dir_or_file, out ? out : &st, cd->handle)) < 0){
		log_debug_ex2("%s: Failed to stat %s", cd->name, dir_or_file);
	}
	return res;
}

int cloud_rename(const char* _old, const char* _new, struct cloud_data* cd){
	if (cd->cf->stat(_old, NULL, cd->handle) != 0){
		log_debug_ex2("%s: File to be renamed (%s) does not exist.", cd->name, _old);
		return -1;
	}
	if (cd->cf->stat(_new, NULL, cd->handle) == 0){
		log_debug_ex2("%s: Destination of rename (%s) already exists.", cd->name, _new);
		return -1;
	}

	if (cd->cf->rename(_old, _new, cd->handle) != 0){
		log_warning_ex("%s: Failed to rename file", cd->name);
		return -1;
	}

	return 0;
}

static int cloud_readdir_choosedir(const char* base_dir, char** output, struct cloud_data* cd){
	char** entries = NULL;
	size_t entries_len = 0;
	char* current_directory = NULL;
	int ret = 0;
	int do_continue = 0;

	current_directory = sh_dup(base_dir);
	*output = NULL;

	do{
		struct string_array* menu_entries = NULL;
		int res;

		do_continue = 0;
		free(*output);
		*output = NULL;
		free_entries(entries, entries_len);
		entries = NULL;
		entries_len = 0;

		if (cd->cf->readdir(current_directory, &entries, &entries_len, cd->handle) != 0){
			log_error("Failed to read directory");
			ret = -1;
			goto cleanup;
		}

		if ((menu_entries = create_menu_entries(current_directory, &entries, &entries_len, cd->handle, cd->cf, 1)) == NULL){
			log_error("Failed to create menu entries");
			ret = -1;
			goto cleanup;
		}
		sa_add(menu_entries, "[Make directory]");

		res = display_menu((const char* const*)menu_entries->strings, menu_entries->len, current_directory);
		sa_free(menu_entries);
		/* make directory is the last menu_entry, it is over the end of entries */
		if ((size_t)res >= entries_len){
			if (cloud_mkdir_ui(current_directory, NULL, cd) != 0){
				log_warning("Failed to create directory");
			}
			do_continue = 1;
			continue;
		}

		/* exit chosen or error */
		if (!entries[res]){
			ret = 1;
			break;
		}
		/* select current directory is entries[0] */
		if (res > 0){
			free(current_directory);
			current_directory = sh_dup(entries[res]);
			do_continue = 1;
			continue;
		}
		else{
			*output = sh_dup(entries[res]);
		}
	}while (do_continue || !confirm_dialog(*output, 1, 0, cd->handle, cd->cf));
cleanup:
	if (ret != 0){
		free(*output);
		*output = NULL;
	}
	free_entries(entries, entries_len);
	free(current_directory);
	return ret;
}

static int cloud_readdir_choosefile(const char* base_dir, char** output, void* handle, const struct cloud_functions* cf){
	char** entries = NULL;
	size_t entries_len = 0;
	uint64_t out_file_size = 0;
	char* current_directory = NULL;
	int ret = 0;
	int do_continue = 0;

	current_directory = sh_dup(base_dir);
	*output = NULL;

	do{
		struct string_array* menu_entries = NULL;
		struct stat st;
		int res;

		do_continue = 0;
		free(*output);
		*output = NULL;
		free_entries(entries, entries_len);
		entries = NULL;
		entries_len = 0;
		sa_free(menu_entries);
		menu_entries = NULL;

		if (cf->readdir(current_directory, &entries, &entries_len, handle) != 0){
			log_error("Failed to read directory");
			ret = -1;
			goto cleanup;
		}

		if ((menu_entries = create_menu_entries(current_directory, &entries, &entries_len, handle, cf, 0)) == NULL){
			log_error("Failed to create menu entries");
			ret = -1;
			goto cleanup;
		}

		res = display_menu((const char* const*)menu_entries->strings, menu_entries->len, current_directory);
		sa_free(menu_entries);

		/* exit chosen or error */
		if (!entries[res]){
			ret = 1;
			break;
		}

		if (cf->stat(entries[res], &st, handle) != 0){
			log_warning_ex("Failed to stat %s", entries[res]);
		}
		out_file_size = st.st_size;

		if (S_ISDIR(st.st_mode)){
			free(current_directory);
			current_directory = sh_dup(entries[res]);
			do_continue = 1;
			continue;
		}
		else{
			*output = sh_dup(entries[res]);
		}
	}while (do_continue || !confirm_dialog(*output, 0, out_file_size, handle, cf));
cleanup:
	if (ret != 0){
		free(*output);
		*output = NULL;
	}
	free_entries(entries, entries_len);
	free(current_directory);
	return ret;
}

int cloud_login(const struct cloud_options* co, struct cloud_data** out_cd){
	int ret = 0;
	int co_contains_username = co->username != NULL;
	int co_contains_password = co->password != NULL;
	char* user = NULL;
	char* pw = NULL;
	struct cloud_data* cd = NULL;

	if (!co_contains_username && (user = read_username_stdin()) == NULL){
		log_info("Blank username specified");
		ret = 0;
		goto cleanup;
	}

	if (!co_contains_password && (pw = read_password_stdin()) == NULL){
		log_info("Blank password specified");
		ret = 0;
		goto cleanup;
	}

	cd = malloc(sizeof(*cd));
	if (!cd){
		log_enomem();
		ret = -1;
		goto cleanup;
	}

	cd->name = cloud_provider_to_string(co->cp);

	cd->cf = cloud_provider_to_cloud_functions(co->cp);
	if (!cd->cf){
		log_error("Failed to determine cloud functions");
		ret = -1;
		goto cleanup;
	}

	if (cd->cf->login(co_contains_username ? co->username : user,
				co_contains_password ? co->password : pw,
				&(cd->handle)) != 0){
		log_error_ex("Failed to log in to %s", cd->name);
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (ret == 0){
		*out_cd = cd;
	}
	else{
		*out_cd = NULL;
		cloud_logout(cd);
	}
	free(user);
	free(pw);
	return ret;
}

int cloud_upload(const char* in_file, const char* upload_dir, struct cloud_data* cd){
	struct string_array* parent_dirs = sa_get_parent_dirs(upload_dir);
	char* progress_msg = sh_sprintf("%s: Uploading %s to %s...", cd->name, in_file, upload_dir);
	int ret = 0;

	if (!parent_dirs){
		log_error("Failed to create parent dir string_array");
		ret = -1;
		goto cleanup;
	}

	if (!progress_msg){
		log_warning("Failed to make progress message");
	}

	if (cd->cf->upload(in_file, upload_dir, progress_msg ? progress_msg : "Uploading file...", cd->handle) != 0){
		log_error_ex2("%s: Failed to upload %s", cd->name, in_file);
		ret = -1;
		goto cleanup;
	}

cleanup:
	sa_free(parent_dirs);
	free(progress_msg);
	return ret;
}

int cloud_upload_ui(const char* in_file, const char* base_path, char** chosen_path, struct cloud_data* cd){
	struct string_array* parent_dirs = sa_get_parent_dirs(base_path);
	char* upload_dir = NULL;
	char* progress_msg = sh_sprintf("%s: Uploading %s to %s...", cd->name, in_file, base_path);
	int ret = 0;
	int res;

	if (!parent_dirs){
		log_error("Failed to create parent dir string_array");
		ret = -1;
		goto cleanup;
	}

	if (!progress_msg){
		log_warning("Failed to make progress message");
	}

	res = cloud_readdir_choosedir(base_path, &upload_dir, cd);
	if (res < 0){
		log_error("Failed to choose a file");
		ret = -1;
		goto cleanup;
	}
	else if (res > 0){
		log_info("Did not choose a file");
		ret = 1;
		goto cleanup;
	}

	if (cd->cf->upload(in_file, upload_dir, progress_msg ? progress_msg : "Uploading file...", cd->handle) != 0){
		log_error_ex2("%s: Failed to upload %s", cd->name, in_file);
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (chosen_path){
		if (ret == 0){
			*chosen_path = upload_dir;
		}
		else{
			free(upload_dir);
		}
	}
	else{
		free(upload_dir);
	}
	sa_free(parent_dirs);
	free(progress_msg);
	return ret;
}

int cloud_download(const char* download_path, char** out_file, struct cloud_data* cd){
	char* progress_msg = NULL;
	int ret = 0;

	if (!(*out_file)){
		*out_file = sh_concat_path(sh_getcwd(), sh_filename(download_path));
		if (!(*out_file)){
			log_error("Failed to determine output file");
			ret = -1;
			goto cleanup;
		}
	}

	progress_msg = sh_sprintf("%s: Downloading %s to %s...", cd->name, download_path, *out_file);
	if (!progress_msg){
		log_warning("Failed to create progress message");
	}

	if (cd->cf->download(download_path, *out_file, progress_msg ? progress_msg : "Downloading file...", cd->handle) != 0){
		log_error_ex2("%s: Failed to download %s", cd->name, download_path);
		ret = -1;
		goto cleanup;
	}

cleanup:
	free(progress_msg);
	return ret;
}

int cloud_download_ui(const char* base_dir, char** out_file, struct cloud_data* cd){
	char* download_file = NULL;
	char* progress_msg = NULL;
	int res;
	int ret = 0;

	res = cloud_readdir_choosefile(base_dir, &download_file, cd->handle, cd->cf);
	if (res < 0){
		log_error("Failed to choose a download file");
		ret = -1;
		goto cleanup;
	}
	else if (res > 0){
		log_info("Download file not chosen");
		ret = 1;
		goto cleanup;
	}

	if (!(*out_file)){
		*out_file = sh_concat_path(sh_getcwd(), sh_filename(base_dir));
		if (!(*out_file)){
			log_error("Failed to determine output file");
			ret = -1;
			goto cleanup;
		}
	}

	progress_msg = sh_sprintf("%s: Downloading %s to %s...", cd->name, download_file, *out_file);
	if (!progress_msg){
		log_warning("Failed to create progress message");
	}

	if (cd->cf->download(download_file, *out_file, progress_msg ? progress_msg : "Downloading file...", cd->handle) != 0){
		log_error_ex2("%s: Failed to download %s", cd->name, download_file);
		ret = -1;
		goto cleanup;
	}

cleanup:
	free(download_file);
	free(progress_msg);
	return ret;
}

int cloud_remove(const char* dir_or_file, struct cloud_data* cd){
	if (cd->cf->remove(dir_or_file, cd->handle) != 0){
		log_warning_ex2("%s: Failed to remove %s", cd->name, dir_or_file);
		return -1;
	}
	return 0;
}

int cloud_remove_ui(const char* base_dir, char** chosen_file, struct cloud_data* cd){
	char* remove_file = NULL;
	int res;
	int ret = 0;

	res = cloud_readdir_choosefile(base_dir, &remove_file, cd->handle, cd->cf);

	if (res < 0){
		log_error("Failed to choose a file to remove");
		ret = -1;
		goto cleanup;
	}
	else if (res > 0){
		log_info("Remove file not chosen");
		ret = 1;
		goto cleanup;
	}

	if (cd->cf->remove(remove_file, cd->handle) != 0){
		log_error_ex2("%s: Failed to remove %s", cd->name, remove_file);
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (chosen_file){
		if (ret == 0){
			*chosen_file = remove_file;
		}
		else{
			free(remove_file);
		}
	}
	else{
		free(remove_file);
	}
	return ret;
}

int cloud_logout(struct cloud_data* cd){
	if (!cd){
		return 0;
	}
	if (cd->cf->logout(cd->handle) != 0){
		log_warning_ex("%s: Failed to logout", cd->name);
		free(cd);
		return -1;
	}
	free(cd);
	return 0;
}
