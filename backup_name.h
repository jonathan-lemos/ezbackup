/* backup_name.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __BACKUP_NAME_H
#define __BACKUP_NAME_H

#include "options/options.h"

char* get_output_name(const char* dir);
char* get_name_intar(const struct options* opt);

#endif
