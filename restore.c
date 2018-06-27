/* restore.c -- restore backend
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "options/options.h"
#include "log.h"
#include "cloud/include.h"
#include "maketar.h"
#include "coredumps.h"
#include "crypt/crypt_easy.h"
#include "filehelper.h"
#include "strings/stringhelper.h"
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

int restore_local_menu(const struct options* opt, char** out_file){
	int res;
	struct file_node** nodes = NULL;
	size_t nodes_size = 0;
	DIR* directory = NULL;
	struct dirent* entry = NULL;
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

cleanup:
	directory ? closedir(directory) : 0;
	free_file_nodes(nodes, nodes_size);
	return ret;
}

int restore_cloud(const struct options* opt, char** out_file){
	struct cloud_options* co = opt->cloud_options;
	char* cwd = NULL;
	int ret = 0;

	cwd = sh_getcwd();
	if (!cwd){
		ret = -1;
		goto cleanup;
	}

	if (cloud_download(cwd, co, out_file) != 0){
		log_debug("Failed to download file from cloud");
		ret = -1;
		goto cleanup;
	}

cleanup:
	free(cwd);
	return ret;
}

int restore_main(const struct options* opt, char** out_file){
	if (opt->cloud_options->cp == CLOUD_NONE || opt->cloud_options->cp == CLOUD_INVALID){
		return restore_local_menu(opt, out_file);
	}

	return restore_cloud(opt, out_file);
}

int restore_extract(const char* file, const struct options* opt){
	char* cwd = NULL;
	char* last_backup_dir = NULL;
	char* last_backup_cfg = NULL;
	struct options* opt_tar = NULL;
	struct TMPFILE* tfp_decrypt = NULL;
	int ret = 0;

	cwd = sh_getcwd();
	if (!cwd){
		log_error("Failed to determine current working directory");
		ret = -1;
		goto cleanup;
	}

	if (get_last_backup_dir(&last_backup_dir) != 0){
		log_error("Failed to determine location of last backup");
		ret = -1;
		goto cleanup;
	}

	last_backup_cfg = sh_concat(sh_dup(cwd), "/config");
	if (!last_backup_cfg){
		log_error("Failed to extract config");
		ret = -1;
		goto cleanup;
	}

	if (opt->enc_algorithm){
		tfp_decrypt = temp_fopen();
		if (!tfp_decrypt){
			log_debug("Failed to make template_decrypt");
			ret = -1;
			goto cleanup;
		}
		if (easy_decrypt(file, tfp_decrypt->name, opt->enc_algorithm, opt->flags.bits.flag_verbose) != 0){
			log_debug("Failed to easy_decrypt()");
			ret = -1;
			goto cleanup;
		}
		temp_fflush(tfp_decrypt);

		if (tar_extract_file(tfp_decrypt->name, "/config", last_backup_cfg) != 0){
			log_debug("Failed to extract config file from tar");
			ret = -1;
			goto cleanup;
		}

		if (parse_options_fromfile(last_backup_dir, &opt_tar) != 0){
			log_error("Failed to extract options from last backup");
			ret = -1;
			goto cleanup;
		}
	}
	else{
		if (tar_extract_file(file, "/config", last_backup_cfg) != 0){
			log_debug("Failed to extract config file from tar");
			ret = -1;
			goto cleanup;
		}

		if (parse_options_fromfile(last_backup_dir, &opt_tar) != 0){
			log_error("Failed to extract options from last backup");
			ret = -1;
			goto cleanup;
		}
	}

cleanup:
	free(cwd);
	free(last_backup_dir);
	free(last_backup_cfg);
	options_free(opt_tar);
	if (tfp_decrypt){
		temp_fclose(tfp_decrypt);
	}
	return ret;
}
