/* include.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CLOUD_INCLUDE_H
#define __CLOUD_INCLUDE_H

#include "cloud_base.h"
#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

enum CLOUD_PROVIDER{
	CLOUD_NONE = 0,
	CLOUD_MEGA = 1,
	CLOUD_INVALID = 2
};

struct cloud_options{
	enum CLOUD_PROVIDER cp;
	char*               username;
	char*               password;
	char*               upload_directory;
};

struct cloud_options* co_new(void);
int co_set_username(struct cloud_options* co, const char* username);
int co_set_password(struct cloud_options* co, const char* password);
int co_set_upload_directory(struct cloud_options* co, const char* upload_directory);
int co_set_default_upload_directory(struct cloud_options* co);
int co_set_cp(struct cloud_options* co, enum CLOUD_PROVIDER cp);
enum CLOUD_PROVIDER cloud_provider_from_string(const char* str);
const char* cloud_provider_to_string(enum CLOUD_PROVIDER cp);
void co_free(struct cloud_options* co);
int co_cmp(const struct cloud_options* co1, const struct cloud_options* co2);

int time_menu(struct file_node** arr, size_t len);
int cloud_upload(const char* in_file, struct cloud_options* co);
int cloud_download(const char* out_dir, struct cloud_options* co, char** out_file);
int cloud_rm(const char* path, struct cloud_options* co);
void free_file_nodes(struct file_node** nodes, size_t len);

#ifdef __UNIT_TESTING__
int cmp(const void* tm1, const void* tm2);
int get_parent_dirs(const char* in, char*** out, size_t* out_len);
int mega_upload(const char* file, const char* upload_dir, const char* username, const char* password);
int mega_download(const char* download_dir, const char* out_file, const char* username, const char* password);
int mega_rm(const char* path, const char* username, const char* password);
#endif

#endif
