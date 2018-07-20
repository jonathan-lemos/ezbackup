/** @file cloud/cloud_options.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "cloud_options.h"
#include "../crypt/crypt_getpassword.h"
#include "../log.h"
#include "../readline_include.h"
#include "../strings/stringhelper.h"
#include <stdlib.h>
#include <string.h>

struct cloud_options* co_new(void){
	struct cloud_options* ret;
	ret = calloc(1, sizeof(*ret));
	if (!ret){
		log_enomem();
		return NULL;
	}
	co_set_default_upload_directory(ret);
	return ret;
}

int co_set_username(struct cloud_options* co, const char* username){
	free(co->username);

	if (!username || strlen(username) == 0){
		co->username = NULL;
		return 0;
	}
	co->username = sh_dup(username);
	if (!co->username){
		log_enomem();
		return -1;
	}
	return 0;
}

int co_set_username_stdin(struct cloud_options* co){
	char* tmp;
	int ret;

	tmp = readline("Username:");
	ret = co_set_username(co, tmp);
	free(tmp);

	return ret;
}

int co_set_password(struct cloud_options* co, const char* password){
	if (co->password){
		free(co->password);
	}
	if (!password || strlen(password) == 0){
		co->password = NULL;
		return 0;
	}
	co->password = sh_dup(password);
	if (!co->password){
		log_enomem();
		return -1;
	}
	return 0;
}

int co_set_password_stdin(struct cloud_options* co){
	char* tmp;
	int res;

	do{
		res = crypt_getpassword("Password:", "Verify password:", &tmp);
	}while (res > 0);
	if (res < 0){
		log_error("Error reading password from terminal");
		crypt_freepassword(tmp);
		return -1;
	}

	res = co_set_password(co, tmp);
	crypt_freepassword(tmp);

	return res;
}

int co_set_upload_directory(struct cloud_options* co, const char* upload_directory){
	if (co->upload_directory){
		free(co->upload_directory);
		co->upload_directory = NULL;
	}
	if (!upload_directory){
		co->upload_directory = NULL;
		return 0;
	}
	co->upload_directory = malloc(strlen(upload_directory) + 1);
	if (!co->upload_directory){
		log_enomem();
		return -1;
	}
	strcpy(co->upload_directory, upload_directory);
	return 0;
}

int co_set_default_upload_directory(struct cloud_options* co){
	return co_set_upload_directory(co, "/Backups");
}

int co_set_cp(struct cloud_options* co, enum CLOUD_PROVIDER cp){
	co->cp = cp;
	return 0;
}

enum CLOUD_PROVIDER cloud_provider_from_string(const char* str){
	enum CLOUD_PROVIDER ret = CLOUD_INVALID;
	if (!strcmp(str, "mega") ||
			!strcmp(str, "MEGA") ||
			!strcmp(str, "mega.nz") ||
			!strcmp(str, "mega.co.nz")){
		ret = CLOUD_MEGA;
	}
	else if (!strcmp(str, "none") ||
			!strcmp(str, "off")){
		ret = CLOUD_NONE;
	}
	else{
		log_warning_ex("Invalid --cloud option chosen (%s)", str);
		ret = CLOUD_INVALID;
	}
	return ret;
}

const char* cloud_provider_to_string(enum CLOUD_PROVIDER cp){
	switch (cp){
	case CLOUD_NONE:
		return "None";
	case CLOUD_MEGA:
		return "mega.nz";
	default:
		log_einval_u(cp);
		return NULL;
	}
}

void co_free(struct cloud_options* co){
	free(co->password);
	free(co->username);
	free(co->upload_directory);
	free(co);
}

int co_cmp(const struct cloud_options* co1, const struct cloud_options* co2){
	if (co1->cp != co2->cp){
		return co1->cp - co2->cp;
	}
	if (sh_cmp_nullsafe(co1->username, co2->username) != 0){
		return sh_cmp_nullsafe(co1->username, co2->username);
	}
	if (sh_cmp_nullsafe(co1->password, co2->password) != 0){
		return sh_cmp_nullsafe(co1->password, co2->password);
	}
	if (sh_cmp_nullsafe(co1->upload_directory, co2->upload_directory) != 0){
		return sh_cmp_nullsafe(co1->upload_directory, co2->upload_directory);
	}
	return 0;
}


