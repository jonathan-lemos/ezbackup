/* restore.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __RESTORE_H
#define __RESTORE_H

#include "options/options.h"

int restore_local_menu(const struct options* opt, char** out_file, char** out_config);
int restore_cloud(const struct options* opt, char** out_file, char** out_config);
int restore_extract(const char* file, const char* config_file);

#ifdef __UNIT_TESTING__
char* getcwd_malloc(void);
#endif

#endif
