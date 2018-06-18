/* fileiterator.c -- recursive file iterator
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* prototypes */
#include "fileiterator.h"

#include "log.h"
/* enumerates files in directory */
#include <dirent.h>
/* holds file permissions and checks if file is actually a directory */
#include <sys/stat.h>
/* strcmp/strcat */
#include <string.h>
/* errno */
#include <errno.h>
/* malloc */
#include <stdlib.h>

struct directory{
	DIR*           dp;
	char*          name;
	struct dirent* dnt;
};

static struct directory** dir_stack = NULL;
static size_t dir_stack_len = 0;

static void free_directory(struct directory* dir){
	dir->dp ? closedir(dir->dp) : 0;
	free(dir->name);
	free(dir);
}

static int directory_pop(void){
	free_directory(dir_stack[dir_stack_len - 1]);
	if (dir_stack_len > 0){
		dir_stack_len--;
	}
	else{
		log_info("Directory stack is empty");
	}
	dir_stack = realloc(dir_stack, sizeof(*dir_stack) * dir_stack_len);
	if (!dir_stack && dir_stack_len > 0){
		log_enomem();
		return -1;
	}
	return 0;
}

static int directory_push(const char* dir){
	dir_stack_len++;
	dir_stack = realloc(dir_stack, sizeof(*dir_stack) * dir_stack_len);
	if (!dir_stack){
		log_enomem();
		return -1;
	}

	dir_stack[dir_stack_len - 1] = calloc(1, sizeof(**dir_stack));
	if (!dir_stack[dir_stack_len - 1]){
		log_enomem();
		return -1;
	}

	dir_stack[dir_stack_len - 1]->dp = opendir(dir);
	if (!dir_stack[dir_stack_len - 1]->dp){
		log_error_ex2("Failed to open %s (%s)", dir, strerror(errno));
		if (directory_pop() != 0){
			log_error("Failed to pop failed directory off stack");
		}
		return -1;
	}

	dir_stack[dir_stack_len - 1]->name = malloc(strlen(dir) + 1);
	if (!dir_stack[dir_stack_len - 1]->name){
		log_error_ex("Failed to allocate space for directory name (%s)", dir);
		if (directory_pop() != 0){
			log_error("Failed to pop failed directory off stack");
		}
		return -1;
	}
	strcpy(dir_stack[dir_stack_len - 1]->name, dir);

	dir_stack[dir_stack_len - 1]->dnt = NULL;
	return 0;
}

static struct directory* directory_peek(void){
	if (!dir_stack){
		return NULL;
	}
	return dir_stack[dir_stack_len - 1];
}

int fi_start(const char* dir){
	if (dir_stack){
		log_error("fi_start_dir() called twice");
		return -1;
	}

	if (directory_push(dir) != 0){
		log_error("Failed to start file iterator");
		return -1;
	}

	return 0;
}

char* fi_get_next(void){
	char* path = NULL;
	struct directory* dir = NULL;
	struct stat st;

	dir = directory_peek();
	if (!dir){
		log_info("Directory stack is empty");
		return NULL;
	}

	dir->dnt = readdir(dir->dp);
	if (!dir->dnt){
		log_info_ex("Out of directory entries in %s", dir->name);
		directory_pop();
		return fi_get_next();
	}

	if (!strcmp(dir->dnt->d_name, ".") || !strcmp(dir->dnt->d_name, "..")){
		return fi_get_next();
	}

	/* generate path to directory */
	/* +3: +1 for each '\0', +1 for '/' */
	path = malloc(strlen(dir->name) + strlen(dir->dnt->d_name) + 3);
	if (!path){
		log_enomem();
		return (char*)-1;
	}
	strcpy(path, dir->name);
	/* append filename */
	if (path[strlen(path) - 1] != '/'){
		strcat(path, "/");
	}
	strcat(path, dir->dnt->d_name);

	/* lstat does not follow symlinks unlike stat */
	/* XOPEN extension */
	lstat(path, &st);
	/* check if file is actually a directory */
	if (S_ISDIR(st.st_mode)){
		/* if so, recursively enum files on that dir */
		directory_push(path);
		return fi_get_next();
	}

	return path;
}

int fi_skip_current_dir(void){
	log_info_ex("Skipping current dir (%s)", directory_peek()->name);
	return directory_pop();
}

void fi_end(void){
	size_t i;
	if (!dir_stack){
		return;
	}
	for (i = 0; i < dir_stack_len; ++i){
		free_directory(dir_stack[i]);
	}
	free(dir_stack);
	dir_stack = NULL;
	dir_stack_len = 0;
}
