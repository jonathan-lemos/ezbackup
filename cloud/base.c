/* base.c
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
	const char* name;
	int (*login)   (const char* username, const char* password, void** out_handle);
	int (*mkdir)   (const char* directory, void* handle);
	int (*readdir) (const char* directory, char*** output, size_t* output_len, void* handle);
	int (*stat)    (const char* file_path, struct stat* out, void* handle);
	int (*rename)  (const char* old_path, const char* new_path, void* handle);
	int (*download)(const char* download_path, const char* out_path, const char* progress_msg, void* handle);
	int (*upload)  (const char* in_path, const char* upload_path, const char* progress_msg, void* handle);
	int (*remove)  (const char* file_path, void* handle);
	int (*logout)  (void* handle);
};

const struct cloud_functions CF_MEGA = {
	"MEGA",
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

static char* get_default_out_file(const char* full_path){
	char* ret;

	ret = sh_concat_path(sh_getcwd(), sh_filename(full_path));
	if (!ret){
		log_error("Failed to create output file path");
		return NULL;
	}
	return ret;
}

static char* cloud_readdir(const char* base_dir, const char* username, const char* password, const struct cloud_functions* cf){
	char** entries = NULL;
	size_t entries_len = 0;
	void* handle = NULL;
	char* current_directory = NULL;
	char* ret = NULL;

	if (cf->login(username, password, &handle) != 0){
		log_error_ex("Failed to log in to %s.", cf->name);
		ret = NULL;
		goto cleanup;
	}

	current_directory = sh_dup(base_dir);

	do{
		char** menu_entries = NULL;
		size_t i;

		if (cf->readdir(current_directory, &entries, &entries_len, handle) != 0){
			log_error("Failed to read the directory.");
			ret = NULL;
			goto cleanup;
		}

		for (i = 0; i < entries_len; ++i){

		}
	} while(!ret);

cleanup:
	if (handle && cf->logout(handle) != 0){
		log_warning_ex("Failed to log out of %s.", cf->name);
	}
	free(current_directory);
	if (entries){
		size_t i;
		for (i = 0; i < entries_len; ++i){
			free(entries[i]);
		}
		free(entries);
	}
	return ret;
}

static int cloud_upload_internal(const char* file, const char* upload_dir, const char* username, const char* password, const struct cloud_functions* cf){
	struct string_array* arr = NULL;
	void* mh = NULL;
	int ret = 0;
	size_t i;

	return_ifnull(file, -1);
	return_ifnull(upload_dir, -1);

	if (cf->login(username, password, &mh) != 0){
		log_error_ex("Failed to log in to %s.", cf->name);
		ret = -1;
		goto cleanup;
	}

	if ((arr = sa_get_parent_dirs(upload_dir)) == NULL){
		log_debug("Failed to determine parent directories");
		ret = -1;
		goto cleanup;
	}

	for (i = 0; i < arr->len; ++i){
		if (cf->mkdir(arr->strings[i], mh) < 0){
			log_debug("Failed to create directory on MEGA");
			ret = -1;
			goto cleanup;
		}
	}

	if (cf->upload(file, upload_dir, "Uploading file...", mh) != 0){
		log_debug("Failed to upload file");
		ret = -1;
		goto cleanup;
	}

	if (cf->logout(mh) != 0){
		log_debug("Failed to logout of MEGA");
		ret = -1;
		mh = NULL;
		goto cleanup;
	}
	mh = NULL;

cleanup:
	if (mh && cf->logout(mh) != 0){
		log_debug("Failed to logout of MEGA");
		ret = -1;
	}

	arr ? sa_free(arr) : (void)0;
	return ret;
}

int cloud_download_internal(const char* download_dir, const char* out_dir, const char* username, const char* password, char** out_file, struct cloud_functions* cf){
	char* msg = NULL;
	char** files = NULL;
	size_t len = 0;
	void* mh = NULL;
	int res;
	int ret = 0;

	if (cf->login(username, password, &mh) != 0){
		log_debug("Failed to login to MEGA");
		ret = -1;
		goto cleanup;
	}

	if (cf->readdir(download_dir, &files, &len, mh) != 0){
		printf("Download directory does not exist\n");
		ret = -1;
		goto cleanup;
	}

	res = time_menu(files, len);
	if (res < 0){
		log_error("Invalid option chosen");
		ret = -1;
		goto cleanup;
	}

	if (!out_dir){
		*out_file = get_default_out_file(files[res]->name);
		if (!(*out_file)){
			log_debug("Failed to determine out file");
			ret = -1;
			goto cleanup;
		}
	}
	else{
		*out_file = malloc(strlen(out_dir) + 1 + strlen(files[res]->name) + 1);
		if (!(*out_file)){
			log_enomem();
			ret = -1;
			goto cleanup;
		}
		strcpy(*out_file, out_dir);
		if ((*out_file)[strlen(*out_file) - 1] != '/'){
			strcat(*out_file, "/");
		}
		strcat(*out_file, sh_filename(files[res]->name));
	}

	msg = malloc(strlen(files[res]->name) + strlen(*out_file) + 64);
	if (!msg){
		log_enomem();
		ret = -1;
		goto cleanup;
	}
	sprintf(msg, "Downloading %s to %s...", files[res]->name, *out_file);
	if (MEGAdownload(files[res]->name, *out_file, msg, mh) != 0){
		log_debug_ex("Failed to download %s", files[res]->name);
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (mh && MEGAlogout(mh) != 0){
		log_debug("Failed to log out of MEGA");
	}
	free_file_nodes(files, len);
	free(msg);

	return ret;
}

int mega_rm(const char* path, const char* username, const char* password){
	MEGAhandle* mh;
	if (MEGAlogin(username, password, &mh) != 0){
		log_debug("Failed to log in to MEGA");
		MEGAlogout(mh);
		return -1;
	}
	if (MEGArm(path, mh) != 0){
		log_debug_ex("Failed to remove %s from MEGA", path);
		MEGAlogout(mh);
		return -1;
	}
	if (MEGAlogout(mh) != 0){
		log_debug("Failed to log out of MEGA");
		return -1;
	}
	return 0;
}

int cloud_upload(const char* in_file, struct cloud_options* co){
	int ret = 0;
	int co_contains_username = co->username != NULL;
	int co_contains_password = co->password != NULL;
	char* user = NULL;
	char* pw = NULL;

	if (!co_contains_username){
		char* tmp = NULL;

		do{
			free(user);
			free(tmp);

			user = readline("Username:");
			if (strlen(user) == 0){
				log_info("Blank username specified");
				ret = 0;
				goto cleanup;
			}

			tmp = readline("Verify  :");
		}while (strcmp(user, tmp) != 0);

		free(tmp);
	}

	if (!co_contains_password){
		int res;
		while ((res = crypt_getpassword("Password:", "Verify  :", &pw)) > 0){
			printf("The passwords do not match.");
			free(pw);
		}

		if (strlen(pw) == 0){
			log_info("Blank password specified");
			ret = 0;
			goto cleanup;
		}
	}

	switch (co->cp){
	case CLOUD_NONE:
		break;
	case CLOUD_MEGA:
		ret = mega_upload(in_file, co->upload_directory, co_contains_username ? co->username : user, co_contains_password ? co->password : pw);
		break;
	default:
		log_error("Invalid CLOUD_PROVIDER passed");
		ret = -1;
		break;
	}

cleanup:
	free(user);
	free(pw);
	return ret;
}

int cloud_download(const char* out_dir, struct cloud_options* co, char** out_file){
	int ret = 0;
	int co_contains_username = co->username != NULL;
	int co_contains_password = co->password != NULL;
	char* user = NULL;
	char* pw = NULL;
	char* cwd = NULL;

	if (!out_dir){
		cwd = sh_getcwd();
	}

	if (!co_contains_username){
		char* tmp = NULL;

		do{
			free(user);
			free(tmp);

			user = readline("Username:");
			if (strlen(user) == 0){
				log_info("Blank username specified");
				ret = 0;
				goto cleanup;
			}

			tmp =  readline("Verify  :");
		}while (strcmp(user, tmp) != 0);

		free(tmp);
	}

	if (!co_contains_password){
		int res;
		while ((res = crypt_getpassword("Password:", "Verify  :", &pw)) > 0){
			printf("The passwords do not match.");
			free(pw);
		}

		if (strlen(pw) == 0){
			log_info("Blank password specified");
			ret = 0;
			goto cleanup;
		}
	}

	switch (co->cp){
	case CLOUD_NONE:
		break;
	case CLOUD_MEGA:
		ret = mega_download(co->upload_directory, cwd ? cwd : out_dir, co_contains_username ? co->username : user, co_contains_password ? co->password : pw, out_file);
		break;
	default:
		log_error("Invalid CLOUD_PROVIDER passed");
		ret = -1;
		break;
	}

cleanup:
	free(user);
	free(pw);
	free(cwd);
	return ret;
}

int cloud_rm(const char* path, struct cloud_options* co){
	int ret = 0;
	int co_contains_username = co->username != NULL;
	int co_contains_password = co->password != NULL;
	char* user = NULL;
	char* pw = NULL;

	if (!co_contains_username){
		char* tmp = NULL;

		do{
			free(user);
			free(tmp);

			user = readline("Username:");
			if (strlen(user) == 0){
				log_info("Blank username specified");
				ret = 0;
				goto cleanup;
			}

			tmp = readline("Verify  :");
		}while (strcmp(user, tmp) != 0);

		free(tmp);
	}

	if (!co_contains_password){
		int res;
		while ((res = crypt_getpassword("Password:", "Verify  :", &pw)) > 0){
			printf("The passwords do not match.");
			free(pw);
		}

		if (strlen(pw) == 0){
			log_info("Blank password specified");
			ret = 0;
			goto cleanup;
		}
	}

	switch (co->cp){
	case CLOUD_NONE:
		break;
	case CLOUD_MEGA:
		ret = mega_rm(path, co_contains_username ? co->username : user, co_contains_password ? co->password : pw);
		break;
	default:
		log_error("Invalid CLOUD_PROVIDER passed");
		ret = -1;
		break;
	}

cleanup:
	free(user);
	free(pw);
	return ret;
}
