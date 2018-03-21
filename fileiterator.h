/*
 * Recursive file iteration module
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

#ifndef __FILE_ITERATOR_H
#define __FILE_ITERATOR_H

/* struct dirent */
#include <dirent.h>
/* struct stat */
#include <sys/stat.h>
/* uintptr_t */
#include <stdint.h>

/* Enumerates through all files starting at dir
 *
 * Parameters:
 *     dir          - Directory in which to start iterating files
 *
 *     func         - Function pointer to receive info on file
 *					  Return 0 to stop iterating.
 *
 *     func_params  - Data for func
 *
 *     error        - Function pointer to handle errors.
 *				      Returns 0 to stop iterating.
 *
 */
void enum_files(
		const char* dir,
		int(*func)(const char*, const char*, struct stat*, void*),
		void* func_params,
		int(*error)(const char*, int, void*),
		void* error_params
		);

#endif
