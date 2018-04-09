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

int cmp(const void* tm1, const void* tm2){
	return ((struct file_node*)tm2)->time - ((struct file_node*)tm1)->time;
}

int time_menu(struct file_node* arr, size_t len){
	char** options;
	int res;
	size_t i;

	qsort(arr, len, sizeof(struct file_node), cmp);

	options = malloc(len * sizeof(*options));
	if (!options){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}

	for (i = 0; i < len; ++i){
		char buf[256];
		struct tm t;
		memcpy(&t, localtime(&(arr[i].time)), sizeof(struct tm));
		strftime(buf, sizeof(buf), "%B %d %Y %H:%M %p %Z", &t);

		options[i] = malloc(strlen(buf) + 1);
		if (!options[i]){
			log_fatal(__FL__, STR_ENOMEM);
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
	dir_tok = strtok(dir, "/");
	while (dir_tok != NULL){
		size_t i;
		size_t len_prev = 0;

		for (i = 0; i < (*out_len); ++i){
			len_prev += strlen((*out)[i]);
		}

		(*out_len)++;
		*out = realloc(*out, *out_len * sizeof(**out));
		if (!(*out)){
			(*out_len)--;
			log_fatal(__FL__, STR_ENOMEM);
			ret = -1;
			goto cleanup;
		}

		(*out)[*out_len - 1] = malloc(len_prev + strlen(dir_tok) + 1);
		if (!(*out)[*out_len - 1]){
			(*out_len)--;
			log_fatal(__FL__, STR_ENOMEM);
			ret = -1;
			goto cleanup;
		}

		(*out)[*out_len - 1][0] = '\0';
		for (i = 0; i < (*out_len) - 1; ++i){
			strcat((*out)[*out_len - 1], (*out)[i]);
		}
		strcat((*out)[*out_len - 1], "/");
		strcat((*out)[*out_len - 1], dir_tok);
	}

cleanup:
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

int mega_upload(const char* file, const char* upload_dir, const char* username, const char* password){
	char* uname = NULL;
	char** parent_dirs;
	size_t parent_dirs_len;
	MEGAhandle* mh = NULL;
	int ret = 0;
	size_t i;

	if (!username){
		uname = readline("Enter username:");
	}
	else{
		uname = malloc(strlen(username) + 1);
		if (!uname){
			log_fatal(__FL__, STR_ENOMEM);
			ret = -1;
			goto cleanup;
		}
		strcpy(uname, username);
	}

	if (MEGAlogin(uname, password, &mh) != 0){
		log_debug(__FL__, "Failed to log in to MEGA");
		ret = -1;
		goto cleanup;
	}

	if (get_parent_dirs(upload_dir, &parent_dirs, &parent_dirs_len) != 0){
		log_debug(__FL__, "Failed to determine parent directories");
		ret = -1;
		goto cleanup;
	}

	for (i = 0; i < parent_dirs_len; ++i){
		if (MEGAmkdir(parent_dirs[i], mh) < 0){
			log_debug(__FL__, "Failed to create directory on MEGA");
			ret = -1;
			goto cleanup;
		}
	}

	if (MEGAupload(file, upload_dir, "Uploading file to MEGA", mh) != 0){
		log_debug(__FL__, "Failed to upload file to MEGA");
		ret = -1;
		goto cleanup;
	}

	if (MEGAlogout(mh) != 0){
		log_debug(__FL__, "Failed to logout of MEGA");
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (mh && MEGAlogout(mh) != 0){
		log_debug(__FL__, "Failed to logout of MEGA");
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

int mega_download(const char* download_dir, const char* out_file, const char* username, const char* password){
	char msg[256];
	struct file_node* files = NULL;
	size_t len = 0;
	MEGAhandle* mh = NULL;
	int res;
	int ret = 0;

	if (MEGAlogin(username, password, &mh) != 0){
		log_debug(__FL__, "Failed to login to MEGA");
		ret = -1;
		goto cleanup;
	}

	if (MEGAreaddir(download_dir, &files, &len, mh) != 0){
		printf("Download directory does not exist\n");
		ret = -1;
		goto cleanup;
	}

	res = time_menu(files, len);
	if (res < 0){
		log_error(__FL__, "Invalid option chosen");
		ret = -1;
		goto cleanup;
	}

	sprintf(msg, "Downloading %s...", files[res].name);
	if (MEGAdownload(files[res].name, out_file, msg, mh) != 0){
		log_debug(__FL__, "Failed to download %s", files[res].name);
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (mh && MEGAlogout(mh) != 0){
		log_debug(__FL__, "Failed to log out of MEGA");
	}
	free(files);

	return ret;
}

int cloud_upload(const char* in_file, const char* upload_dir, const char* username, enum CLOUD_PROVIDER cp){
	int ret = 0;

	switch (cp){
	case CLOUD_NONE:
		break;
	case CLOUD_MEGA:
		ret = mega_upload(in_file, upload_dir, username, NULL);
		break;
	default:
		log_error(__FL__, "Invalid CLOUD_PROVIDER passed");
		ret = -1;
		break;
	}

	return ret;
}

int cloud_download(const char* download_dir, const char* out_file, const char* username, enum CLOUD_PROVIDER cp){
	int ret = 0;

	switch (cp){
	case CLOUD_NONE:
		break;
	case CLOUD_MEGA:
		ret = mega_download(download_dir, out_file, username, NULL);
		break;
	default:
		log_error(__FL__, "Invalid CLOUD_PROVIDER passed");
		ret = -1;
		break;
	}

	return ret;
}
