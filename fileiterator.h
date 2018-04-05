/* fileiterator.h -- recursive file iterator
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
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
		int(*func)(const char* file, const char* dir, struct stat* st, void* params),
		void* func_params,
		int(*error)(const char* file, int __errno, void* params),
		void* error_params
		);

#endif
