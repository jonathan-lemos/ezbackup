/* restore.c -- restore backend
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "options.h"
#include "error.h"
#include "cloud/include.h"
#include "maketar.h"
#include "coredumps.h"
#include "crypt_easy.h"
#include "readfile.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#if defined(__linux__)
#include <editline/readline.h>
#elif defined(__APPLE__)
#include <readline/readline.h>
#else
#error "This operating system is not supported"
#endif

char* getcwd_malloc(void){
	char* cwd = NULL;
	int cwd_len = 256;

	cwd = malloc(cwd_len);
	if (!cwd){
		log_enomem();
		return NULL;
	}
	while (getcwd(cwd, cwd_len) == NULL){
		if (errno != ERANGE){
			log_error_ex("Failed to get current directory (%s)", strerror(errno));
			free(cwd);
			return NULL;
		}
		cwd_len *= 2;
		cwd = realloc(cwd, cwd_len);
		if (!cwd){
			log_enomem();
			return NULL;
		}
	}
	cwd = realloc(cwd, strlen(cwd) + 1);
	if (!cwd){
		log_enomem();
		return NULL;
	}

	return cwd;
}

int restore_local_menu(const struct options* opt, char** out_file, char** out_config){
	int res;
	struct file_node** nodes = NULL;
	size_t nodes_size = 0;
	DIR* directory;
	struct dirent* entry;
	int ret = 0;

	directory = opendir(opt->output_directory);
	if (!directory){
		log_error_ex("Failed to open output directory (%s)", strerror(errno));
	}

	while ((entry = readdir(directory)) != NULL){
		char* path;
		struct stat st;

		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")){
			continue;
		}

		path = malloc(strlen(opt->output_directory) + strlen(entry->d_name) + 3);
		if (!path){
			log_enomem();
			ret = -1;
			goto cleanup;
		}
		strcpy(path, opt->output_directory);
		if (path[strlen(path) - 1] != '/'){
			strcat(path, "/");
		}
		strcat(path, entry->d_name);

		if (stat(path, &st) != 0){
			log_estat(path);
			continue;
		}

		nodes_size++;
		nodes = realloc(nodes, nodes_size * sizeof(*nodes));
		if (!nodes){
			log_enomem();
			ret = -1;
			goto cleanup;
		}

		nodes[nodes_size - 1]->name = path;
		nodes[nodes_size - 1]->time = st.st_ctime;
	}

	res = time_menu(nodes, nodes_size);
	if (res < 0){
		log_error("Invalid option chosen");
		ret = -1;
		goto cleanup;
	}
	*out_file = nodes[res]->name;
	nodes[res]->name = NULL;
	*out_config = malloc(strlen(*out_file) + sizeof(".conf"));
	if (!(*out_config)){
		log_enomem();
		ret = -1;
		goto cleanup;
	}
	strcpy(*out_config, *out_file);
	strcat(*out_config, ".conf");
cleanup:
	free_file_nodes(nodes, nodes_size);
	return ret;
}

int restore_cloud(const struct options* opt, char** out_file, char** out_config){
	char* uname = NULL;
	if (!opt->username){
		uname = readline("Enter username:");
	}
	if (cloud_download(opt->upload_dir, NULL, uname ? uname : opt->username, opt->password, opt->cp, out_file, out_config) != 0){
		log_debug("Failed to download file from cloud");
		if (uname){
			free(uname);
		}
		return -1;
	}
	if (uname){
		free(uname);
	}
	return 0;
}

int restore_choose_directory(char** out_dir){
	printf("Current directory: %s\n", *out_dir ? *out_dir : "default");
	*out_dir = readline("Enter output directory:");
	if (strcmp(*out_dir, "") == 0){
		free(*out_dir);
		*out_dir = NULL;
	}
	return 0;
}

int restore_main_menu(const struct options* opt, char** out_file, char** out_config){
	int res;
	int ret = 0;
	const char* options_main[] = {
		"Restore locally",
		"Restore from cloud",
		"Choose restore directory (cloud)",
		"Exit"
	};
	char* out_dir = NULL;

	do{
		res = display_menu(options_main, sizeof(options_main) / sizeof(options_main[0]), "Restore from where?");
		switch (res){
		case 0:
			ret = restore_local_menu(opt, out_file, out_config);
			break;
		case 1:
			ret = restore_cloud(opt, out_file, out_config);
			break;
		case 2:
			restore_choose_directory(&out_dir);
			break;
		case 3:
			return 0;
		default:
			log_error("Invalid option chosen. This should never happen.");
		}
	}while (res == 2);

	return 0;
}

int restore_extract(const char* file, const char* config_file){
	char* cwd = NULL;
	struct options opt;
	char template_decrypt[] = "/var/tmp/decrypt_XXXXXX";
	FILE* fp_decrypt = NULL;
	int ret = 0;

	memset(&opt, '\0', sizeof(opt));

	cwd = getcwd_malloc();
	if (!cwd){
		log_debug("Failed to determine current working directory");
		ret = -1;
		goto cleanup;
	}

	if (read_config_file(&opt, config_file) != 0){
		log_debug("Failed to read config file for extracting");
		ret = -1;
		goto cleanup;
	}

	if (opt.enc_algorithm){
		fp_decrypt = temp_fopen(template_decrypt);
		if (!fp_decrypt){
			log_debug("Failed to make template_decrypt");
			ret = -1;
			goto cleanup;
		}
		fclose(fp_decrypt);
		fp_decrypt = NULL;
		if (easy_decrypt(file, template_decrypt, opt.enc_algorithm, opt.flags & FLAG_VERBOSE) != 0){
			log_debug("Failed to easy_decrypt()");
			ret = -1;
			goto cleanup;
		}
		remove(file);
		file = template_decrypt;
	}

	if (tar_extract(file, cwd) != 0){
		log_debug("Failed to extract tar");
		ret = -1;
		goto cleanup;
	}

cleanup:
	free(cwd);
	free_options(&opt);
	fp_decrypt ? fclose(fp_decrypt) : 0;
	remove(template_decrypt);
	remove(file);
	return ret;
}
