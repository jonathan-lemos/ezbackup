/* include.h -- cloud support master module
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CLOUD_INCLUDE_H
#define __CLOUD_INCLUDE_H

#include "cloud_base.h"

enum CLOUD_PROVIDER{
	CLOUD_NONE = 0,
	CLOUD_MEGA = 1,
	CLOUD_INVALID = 2
};

int cloud_upload(const char* in_file, const char* upload_dir, const char* username, enum CLOUD_PROVIDER cp);
int cloud_download(const char* download_dir, const char* out_file, const char* username, enum CLOUD_PROVIDER cp);

#ifdef __UNIT_TESTING__
int cmp(const void* tm1, const void* tm2);
int time_nenu(struct file_node* arr, size_t len);
int get_parent_dirs(const char* in, char*** out, size_t out_len);
int mega_upload(const char* file, const char* upload_dir, const char* username, const char* password);
int mega_download(const char* download_dir, const char* out_file, const char* username, const char* password);
#endif

#endif
