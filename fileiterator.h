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
