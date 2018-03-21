/*
 * Recursive file enumeration module
 * Copyright (C) 2018 Jonathan Lemos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "fileiterator.h"
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

void enum_files(
		const char* dir,
		int(*func)(const char*, const char*, struct stat*, void*),
		void* func_params,
		int(*error)(const char*, int, void*),
		void* error_params
		){
	struct dirent* dnt;
	DIR* dp = opendir(dir);

	if (!dp){
		/* return char* to dir so user can see which directory went wrong/needs sudo */
		if (error){
			error(dir, errno, error_params);
		}
		return;
	}

	while ((dnt = readdir(dp)) != NULL){
		char* path;
		struct stat st;

		/* . and .. are not real files or directories */
		if(!strcmp(dnt->d_name, ".") || !strcmp(dnt->d_name, "..")){
			continue;
		}

		/* generate path to directory */
		/* +3: +1 for each '\0', +1 for '/' */
		path = malloc(strlen(dir) + strlen(dnt->d_name) + 3);
		strcpy(path, dir);
		/* append filename */
		if (path[strlen(path) - 1] != '/'){
			strcat(path, "/");
		}
		strcat(path, dnt->d_name);

		/* lstat does not follow symlinks unlike stat */
		/* XOPEN extension */
		lstat(path, &st);
		/* check if file is actually a directory */
		if (S_ISDIR(st.st_mode)){
			/* if so, recursively enum files on that dir */
			enum_files(path, func, func_params, error, error_params);
			free(path);
			continue;
		}

		/* if func returns 0, stop iterating */
		if(!func(path, dir, &st, func_params)){
			closedir(dp);
			free(path);
			return;
		}
		free(path);
	}
	closedir(dp);
}
