/* cloud_options.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CLOUD_OPTIONS_H
#define __CLOUD_OPTIONS_H

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

#endif
