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

int mega_upload(const char* file, const char* upload_dir, const char* username, const char* password){
	char* uname = NULL;
	char* dir = NULL;
	char* dir_tok = NULL;
	MEGAhandle* mh = NULL;
	int ret = 0;

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

	dir = malloc(strlen(upload_dir) + 1);
	if (!dir){
		log_fatal(__FL__, STR_ENOMEM);
		ret = -1;
		goto cleanup;
	}

	dir_tok = strtok(dir, "/");
	while (dir_tok != NULL){
		if (MEGAmkdir(dir_tok, mh) < 0){
			log_debug(__FL__, "Failed to create backup directory in MEGA");
			ret = -1;
			goto cleanup;
		}
		dir_tok = strtok(NULL, "/");
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
	free(dir);
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
