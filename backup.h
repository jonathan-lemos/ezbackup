/* backup.h -- backup backend
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __BACKUP_H
#define __BACKUP_H

#include "options.h"

int backup(struct options* opt, const struct options* opt_prev);

#ifdef __UNIT_TESTING__
int disable_core_dumps(void);
int enable_core_dumps(void);
int extract_prev_checksums(const char* in, const char* out, const EVP_CIPHER* enc_algorithm, int verbose);
int encrypt_file(const char* in, const char* out, const EVP_CIPHER* enc_algorithm, int verbose);
int rename_ex(const char* _old, const char* _new);
int get_default_backup_name(struct options* opt, char** out);
#endif

#endif
