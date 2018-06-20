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

struct fi_stack* fi_start(const char* dir);
char* fi_next(struct fi_stack* fis);
int fi_skip_current_dir(struct fi_stack* fis);
void fi_end(struct fi_stack* fis);

#endif
