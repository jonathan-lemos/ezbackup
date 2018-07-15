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
#include <sys/stat.h>

struct cloud_data;

int cloud_login(struct cloud_options* co, struct cloud_data** out_cd);
int cloud_mkdir(const char* dir, struct cloud_data* cd);
int cloud_mkdir_ui(const char* base_dir, char** chosen_file, struct cloud_data* cd);
int cloud_stat(const char* dir_or_file, struct stat* out, struct cloud_data* cd);
int cloud_upload(const char* in_file, const char* upload_dir, struct cloud_data* cd);
int cloud_upload_ui(const char* in_file, const char* base_dir, char** chosen_path, struct cloud_data* cd);
int cloud_download(const char* download_path, char** out_file, struct cloud_data* cd);
int cloud_download_ui(const char* base_dir, char** out_file, struct cloud_data* cd);
int cloud_remove(const char* dir_or_file, struct cloud_data* cd);
int cloud_remove_ui(const char* base_dir, char** chosen_file, struct cloud_data* cd);
int cloud_logout(struct cloud_data* cd);

#endif
