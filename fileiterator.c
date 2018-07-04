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

struct fi_stack{
	struct directory{
		DIR* dp;
		char* name;
		struct dirent* dnt;
	}**dir_stack;

	size_t dir_stack_len;
};

static void free_directory(struct directory* dir){
	dir->dp ? closedir(dir->dp) : 0;
	free(dir->name);
	free(dir);
}

static int directory_pop(struct fi_stack* fis){
	free_directory(fis->dir_stack[fis->dir_stack_len - 1]);
	if (fis->dir_stack_len > 0){
		fis->dir_stack_len--;
	}
	else{
		log_info("Directory stack is empty");
	}
	fis->dir_stack = realloc(fis->dir_stack, sizeof(*fis->dir_stack) * fis->dir_stack_len);
	if (!fis->dir_stack && fis->dir_stack_len > 0){
		log_enomem();
		return -1;
	}
	return 0;
}

static int directory_push(const char* dir, struct fi_stack* fis){
	fis->dir_stack_len++;
	fis->dir_stack = realloc(fis->dir_stack, sizeof(*fis->dir_stack) * fis->dir_stack_len);
	if (!fis->dir_stack){
		log_enomem();
		return -1;
	}

	fis->dir_stack[fis->dir_stack_len - 1] = calloc(1, sizeof(**fis->dir_stack));
	if (!fis->dir_stack[fis->dir_stack_len - 1]){
		log_enomem();
		return -1;
	}

	fis->dir_stack[fis->dir_stack_len - 1]->dp = opendir(dir);
	if (!fis->dir_stack[fis->dir_stack_len - 1]->dp){
		log_error_ex2("Failed to open %s (%s)", dir, strerror(errno));
		if (directory_pop(fis) != 0){
			log_error("Failed to pop failed directory off stack");
		}
		return -1;
	}

	fis->dir_stack[fis->dir_stack_len - 1]->name = malloc(strlen(dir) + 1);
	if (!fis->dir_stack[fis->dir_stack_len - 1]->name){
		log_error_ex("Failed to allocate space for directory name (%s)", dir);
		if (directory_pop(fis) != 0){
			log_error("Failed to pop failed directory off stack");
		}
		return -1;
	}
	strcpy(fis->dir_stack[fis->dir_stack_len - 1]->name, dir);

	fis->dir_stack[fis->dir_stack_len - 1]->dnt = NULL;
	return 0;
}

static struct directory* directory_peek(const struct fi_stack* fis){
	if (!fis->dir_stack){
		return NULL;
	}
	return fis->dir_stack[fis->dir_stack_len - 1];
}

struct fi_stack* fi_start(const char* dir){
	struct fi_stack* fis = NULL;

	fis = calloc(1, sizeof(*fis));
	if (!fis){
		log_enomem();
		return NULL;
	}

	if (directory_push(dir, fis) != 0){
		log_error("Failed to initialize fi_stack");
		free(fis);
		return NULL;
	}

	return fis;
}

char* fi_next(struct fi_stack* fis){
	char* path = NULL;
	struct directory* dir = NULL;
	struct stat st;

	dir = directory_peek(fis);
	if (!dir){
		log_info("Directory stack is empty");
		return NULL;
	}

	dir->dnt = readdir(dir->dp);
	if (!dir->dnt){
		log_info_ex("Out of directory entries in %s", dir->name);
		directory_pop(fis);
		return fi_next(fis);
	}

	if (!strcmp(dir->dnt->d_name, ".") || !strcmp(dir->dnt->d_name, "..")){
		return fi_next(fis);
	}

	/* generate path to directory */
	/* +3: +1 for each '\0', +1 for '/' */
	path = malloc(strlen(dir->name) + strlen(dir->dnt->d_name) + 3);
	if (!path){
		log_enomem();
		return NULL;
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
		directory_push(path, fis);
		free(path);
		return fi_next(fis);
	}

	return path;
}

int fi_skip_current_dir(struct fi_stack* fis){
	log_info_ex("Skipping current dir (%s)", directory_peek(fis)->name);
	return directory_pop(fis);
}

const char* fi_directory_name(const struct fi_stack* fis){
	return fis && fis->dir_stack_len > 0 ? directory_peek(fis)->name : NULL;
}

void fi_end(struct fi_stack* fis){
	size_t i;
	if (!fis->dir_stack){
		free(fis);
		return;
	}
	for (i = 0; i < fis->dir_stack_len; ++i){
		free_directory(fis->dir_stack[i]);
	}
	free(fis->dir_stack);
	free(fis);
}
