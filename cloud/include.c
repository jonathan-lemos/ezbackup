/* include.c -- cloud support master module
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "include.h"
#include "../options.h"
#include "../error.h"
#include "mega.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <editline/readline.h>
#elif defined(__APPLE__)
#include <readline/readline.h>
#else
#error "This operating system is not supported"
#endif

struct cloud_options* co_new(void){
	struct cloud_options* ret;
	ret = malloc(sizeof(*ret));
	if (!ret){
		log_enomem();
		return NULL;
	}
	ret->cp = CLOUD_NONE;
	ret->username = NULL;
	ret->password = NULL;
	ret->upload_directory = NULL;
	return ret;
}

int co_set_username(struct cloud_options* co, const char* username){
	if (co->username){
		free(co->username);
	}
	if (!username){
		co->username = NULL;
		return 0;
	}
	co->username = malloc(strlen(username) + 1);
	if (!co->username){
		log_enomem();
		return -1;
	}
	strcpy(co->username, username);
	return 0;
}

int co_set_password(struct cloud_options* co, const char* password){
	if (co->password){
		free(co->password);
	}
	if (!password){
		co->password = NULL;
		return 0;
	}
	co->password = malloc(strlen(password) + 1);
	if (!co->password){
		log_enomem();
		return -1;
	}
	strcpy(co->password, password);
	return 0;
}

int co_set_upload_directory(struct cloud_options* co, const char* upload_directory){
	if (co->upload_directory){
		free(co->upload_directory);
	}
	if (!co->upload_directory){
		co->upload_directory = NULL;
		return 0;
	}
	co->upload_directory = malloc(strlen(upload_directory) + 1);
	if (!co->upload_directory){
		log_enomem();
		return -1;
	}
	strcpy(co->upload_directory, upload_directory);
	return 0;
}

int co_set_default_upload_directory(struct cloud_options* co){
	return co_set_upload_directory(co, "/Backups");
}

enum CLOUD_PROVIDER cloud_provider_from_string(const char* str){
	enum CLOUD_PROVIDER ret = CLOUD_INVALID;
	if (!strcmp(str, "mega") ||
			!strcmp(str, "MEGA") ||
			!strcmp(str, "mega.nz") ||
			!strcmp(str, "mega.co.nz")){
		ret = CLOUD_MEGA;
	}
	else if (!strcmp(str, "none") ||
			!strcmp(str, "off")){
		ret = CLOUD_NONE;
	}
	else{
		log_warning_ex("Invalid --cloud option chosen (%s)", str);
		ret = CLOUD_INVALID;
	}
	return ret;
}

const char* cloud_provider_to_string(enum CLOUD_PROVIDER cp){
	switch (cp){
	case CLOUD_NONE:
		return "none";
	case CLOUD_MEGA:
		return "mega.nz";
	default:
		return "invalid";
	}
}

void co_free(struct cloud_options* co){
	free(co->password);
	free(co->username);
	free(co->upload_directory);
	free(co);
}

int cmp(const void* tm1, const void* tm2){
	return (*((struct file_node**)tm2))->time - (*((struct file_node**)tm1))->time;
}

int time_menu(struct file_node** arr, size_t len){
	char** options;
	int res;
	size_t i;

	qsort(arr, len, sizeof(*arr), cmp);

	options = malloc(len * sizeof(*options));
	if (!options){
		log_enomem();
		return -1;
	}

	for (i = 0; i < len; ++i){
		char buf[256];
		struct tm t;
		memcpy(&t, localtime(&(arr[i]->time)), sizeof(struct tm));
		/* January 1 1970 12:00 AM GMT */
		strftime(buf, sizeof(buf), "%B %d %Y %H:%M %p %Z", &t);

		options[i] = malloc(strlen(buf) + 1);
		if (!options[i]){
			log_enomem();
			free(options);
			return -1;
		}
		strcpy(options[i], buf);
	}

	res = display_menu((const char**)options, len, "Select a time to restore from");

	for (i = 0; i < len; ++i){
		free(options[i]);
	}
	free(options);
	return res;
}

int get_parent_dirs(const char* in, char*** out, size_t* out_len){
	char* dir = NULL;
	char* dir_tok = NULL;
	int ret = 0;

	*out = NULL;
	*out_len = 0;

	dir = malloc(strlen(in) + 1);
	strcpy(dir, in);
	dir_tok = strtok(dir, "/");
	while (dir_tok != NULL){
		size_t len_prev = 0;

		if (*out_len > 1){
			len_prev = strlen((*out)[*out_len - 1]);
		}
		else{
			len_prev = 0;
		}

		(*out_len)++;
		*out = realloc(*out, *out_len * sizeof(**out));
		if (!(*out)){
			(*out_len)--;
			log_enomem();
			ret = -1;
			goto cleanup_freeout;
		}

		(*out)[*out_len - 1] = malloc(len_prev + strlen(dir_tok) + 2);
		if (!(*out)[*out_len - 1]){
			(*out_len)--;
			log_enomem();
			ret = -1;
			goto cleanup_freeout;
		}

		(*out)[*out_len - 1][0] = '\0';
		if (*out_len > 1){
			strcat((*out)[*out_len - 1], (*out)[*out_len - 2]);
		}
		strcat((*out)[*out_len - 1], "/");
		strcat((*out)[*out_len - 1], dir_tok);

		dir_tok = strtok(NULL, "/");
	}

	free(dir);
	return ret;

cleanup_freeout:
	free(dir);
	if (*out){
		size_t i;
		for (i = 0; i < (*out_len); ++i){
			free((*out)[i]);
		}
		free(*out);
	}
	return ret;
}

const char* just_the_filename(const char* full_path){
	size_t ptr = 0;
	while((ptr = strcspn(full_path, "/")) != strlen(full_path)){
		full_path += ptr + 1;
		ptr = 0;
	}
	return full_path;
}

char* get_default_out_file(const char* full_path){
	int cwd_len = 256;
	char* cwd;
	const char* filename = just_the_filename(full_path);

	cwd = malloc(cwd_len);
	if (!cwd){
		log_enomem();
		return NULL;
	}

	while (getcwd(cwd, cwd_len) == NULL){
		cwd_len *= 2;
		cwd = realloc(cwd, cwd_len);
		if (!cwd){
			log_enomem();
			return NULL;
		}
	}
	cwd = realloc(cwd, strlen(cwd) + 1 + strlen(filename) + 1);
	if (!cwd){
		log_enomem();
		return NULL;
	}

	if (cwd[strlen(cwd) - 1] != '/'){
		strcat(cwd, "/");
	}
	strcat(cwd, filename);
	return cwd;
}

const char* get_extension(const char* filename){
	size_t ptr = 0;
	while ((ptr = strcspn(filename, ".")) != strlen(filename)){
		filename += ptr + 1;
		ptr = 0;
	}
	return filename;
}

int remove_file_node(struct file_node*** nodes, size_t* len, size_t index){
	size_t i;
	free((*nodes)[index]->name);
	free((*nodes)[index]);
	for (i = index; i < *len - 1; ++i){
		(*nodes)[i] = (*nodes)[i + 1];
	}
	(*len)--;
	*nodes = realloc(*nodes, *len * sizeof(**nodes));
	if (!(*nodes)){
		log_enomem();
		return -1;
	}
	return 0;
}

void free_file_nodes(struct file_node** nodes, size_t len){
	size_t i;
	for (i = 0; i < len; ++i){
		free(nodes[i]->name);
		free(nodes[i]);
	}
	free(nodes);
}

int mega_upload(const char* file, const char* upload_dir, const char* username, const char* password){
	char* uname = NULL;
	char** parent_dirs = NULL;
	size_t parent_dirs_len = 0;
	MEGAhandle* mh = NULL;
	int ret = 0;
	size_t i;

	if (!username){
		uname = readline("Enter username:");
	}
	else{
		uname = malloc(strlen(username) + 1);
		if (!uname){
			log_enomem();
			ret = -1;
			goto cleanup;
		}
		strcpy(uname, username);
	}

	if (MEGAlogin(uname, password, &mh) != 0){
		log_debug("Failed to log in to MEGA");
		ret = -1;
		goto cleanup;
	}

	if (get_parent_dirs(upload_dir, &parent_dirs, &parent_dirs_len) != 0){
		log_debug("Failed to determine parent directories");
		ret = -1;
		goto cleanup;
	}

	for (i = 0; i < parent_dirs_len; ++i){
		if (MEGAmkdir(parent_dirs[i], mh) < 0){
			log_debug("Failed to create directory on MEGA");
			ret = -1;
			goto cleanup;
		}
	}

	if (MEGAupload(file, upload_dir, "Uploading file to MEGA", mh) != 0){
		log_debug("Failed to upload file to MEGA");
		ret = -1;
		goto cleanup;
	}

	if (MEGAlogout(mh) != 0){
		log_debug("Failed to logout of MEGA");
		ret = -1;
		mh = NULL;
		goto cleanup;
	}
	mh = NULL;

cleanup:
	if (mh && MEGAlogout(mh) != 0){
		log_debug("Failed to logout of MEGA");
		ret = -1;
	}

	if (parent_dirs){
		for (i = 0; i < parent_dirs_len; ++i){
			free(parent_dirs[i]);
		}
		free(parent_dirs);
	}

	free(uname);
	return ret;
}

int mega_download(const char* download_dir, const char* out_dir, const char* username, const char* password, char** out_file, char** out_conf){
	char* msg = NULL;
	char* conf_file = NULL;
	struct file_node** files = NULL;
	size_t len = 0;
	MEGAhandle* mh = NULL;
	int res;
	int ret = 0;
	size_t i;

	if (MEGAlogin(username, password, &mh) != 0){
		log_debug("Failed to login to MEGA");
		ret = -1;
		goto cleanup;
	}

	if (MEGAreaddir(download_dir, &files, &len, mh) != 0){
		printf("Download directory does not exist\n");
		ret = -1;
		goto cleanup;
	}

	for (i = 0; i < len; ++i){
		if (strcmp(get_extension(files[i]->name), ".conf") != 0){
			continue;
		}
		if (remove_file_node(&files, &len, i) != 0){
			log_debug("Failed to remove file node");
		}
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
		strcat(*out_file, files[res]->name);
	}

	*out_conf = malloc(strlen(*out_file) + sizeof(".conf"));
	if (!(*out_conf)){
		log_enomem();
		ret = -1;
		goto cleanup;
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

	if (MEGAdownload(conf_file, *out_conf, "Downloading configuration file...", mh) != 0){
		log_warning("Configuration file not found.");
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

int cloud_upload(const char* in_file, const char* upload_dir, const char* username, const char* password, enum CLOUD_PROVIDER cp){
	int ret = 0;

	switch (cp){
	case CLOUD_NONE:
		break;
	case CLOUD_MEGA:
		ret = mega_upload(in_file, upload_dir, username, password);
		break;
	default:
		log_error("Invalid CLOUD_PROVIDER passed");
		ret = -1;
		break;
	}

	return ret;
}

int cloud_download(const char* download_dir, const char* out_dir, const char* username, const char* password, enum CLOUD_PROVIDER cp, char** out_file, char** out_conf){
	int ret = 0;

	switch (cp){
	case CLOUD_NONE:
		break;
	case CLOUD_MEGA:
		ret = mega_download(download_dir, out_dir, username, password, out_file, out_conf);
		break;
	default:
		log_error("Invalid CLOUD_PROVIDER passed");
		ret = -1;
		break;
	}

	return ret;
}

int cloud_rm(const char* path, const char* username, const char* password, enum CLOUD_PROVIDER cp){
	int ret = 0;

	switch (cp){
	case CLOUD_NONE:
		break;
	case CLOUD_MEGA:
		ret = mega_rm(path, username, password);
		break;
	default:
		log_error("Invalid CLOUD_PROVIDER passed");
		ret = -1;
		break;
	}

	return ret;
}
