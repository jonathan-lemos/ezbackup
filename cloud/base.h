/* base.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CLOUD_BASE_H
#define __CLOUD_BASE_H

#include "cloud_options.h"

int cloud_upload(const char* in_file, struct cloud_options* co);
int cloud_download(const char* out_dir, struct cloud_options* co, char** out_file);
int cloud_rm(const char* path, struct cloud_options* co);

#endif
