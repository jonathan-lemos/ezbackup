/* backup.h -- backup backend
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __BACKUP_H
#define __BACKUP_H

#include "options/options.h"

int backup(const struct options* opt);

#ifdef __UNIT_TESTING__
#include "filehelper.h"
int create_tar_from_directories(const struct options* opt, FILE* fp_hashes, FILE* fp_hashes_prev, const char* out);
struct TMPFILE* extract_prev_checksums(const char* in);
int create_final_tar(const char* tar_out, const char* tar_in, const char* file_hashes, const char* file_hashes_prev, const struct options* opt, int verbose);
int backup(const struct options* opt);
#endif

#endif
