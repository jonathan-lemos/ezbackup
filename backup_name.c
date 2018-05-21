/* backup_name.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "error.h"
#include "maketar.h"
#include "options/options.h"
#include "strings/stringhelper.h"
#include <string.h>
#include <time.h>

static const char* get_archive_extension(void){
	return ".tar";
}

static const char* get_compression_extension(enum COMPRESSOR comp){
	switch (comp){
		case COMPRESSOR_GZIP:
			return ".gz";
		case COMPRESSOR_BZIP2:
			return ".bz2";
		case COMPRESSOR_XZ:
			return ".xz";
		case COMPRESSOR_LZ4:
			return ".lz4";
		case COMPRESSOR_NONE:
			return "";
		default:
			log_einval(comp);
			return NULL;
	}
}

static const char* get_backup_extension(const struct options* opt){
	static char ext[128];

	strcpy(ext, get_archive_extension());

	if (opt->comp_algorithm != COMPRESSOR_NONE){
		strcat(ext, get_compression_extension(opt->comp_algorithm));
	}

	if (strcmp(EVP_CIPHER_name(opt->enc_algorithm), "NULL") != 0){
		strcat(ext, EVP_CIPHER_name(opt->enc_algorithm));
	}

	return ext;
}

static char* get_default_backup_base(const char* dir){
	char file[64];
	char* ret;

	ret = sh_dup(dir);
	if (!ret){
		log_enomem();
		return NULL;
	}
	if (ret[strlen(ret) - 1] == '/'){
		ret[strlen(ret) - 1] = '\0';
	}

	sprintf(file, "/backup-%ld", (long)time(NULL));
	ret = sh_concat(ret, file);
	return ret;
}

char* get_output_name(const char* dir){
	char* ret;

	ret = sh_concat(get_default_backup_base(dir), get_archive_extension());
	if (!ret){
		log_error("Failed to concatenate extension to base");
		return NULL;
	}

	return ret;
}

char* get_name_intar(const struct options* opt){
	char* ret;

	ret = sh_concat("/files-", get_backup_extension(opt));
	if (!ret){
		log_error("Failed to concatenate extension to base");
		return NULL;
	}

	return ret;
}
