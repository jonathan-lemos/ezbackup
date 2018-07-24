/** @file options/options_menu.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "options.h"
#include "../cli.h"
#include "../crypt/crypt.h"
#include "../crypt/crypt_getpassword.h"
#include "../log.h"
#include "../strings/stringhelper.h"
#include "../readline_include.h"
#include <string.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static char* option_subtitle(const char* option, const char* subtitle){
	char* ret = NULL;

	if (!subtitle){
		ret = sh_dup(option);
		if (!ret){
			log_error("Failed to create subtitle");
		}
		return ret;
	}

	ret = sh_sprintf("%s (%s)", option, subtitle);
	if (!ret){
		log_error("Failed to create subtitle");
	}
	return ret;
}

static char* option_subtitle_passwd(const char* option, const char* passwd){
	char* ret = NULL;
	char* buf = NULL;
	size_t i;

	if (passwd){
		buf = malloc(strlen(passwd) + 1);
		if (!buf){
			log_error("Failed to allocate memory for asterisk buffer");
			return NULL;
		}

		for (i = 0; i < strlen(passwd); ++i){
			buf[i] = '*';
		}
		buf[strlen(passwd)] = '\0';
		ret = option_subtitle(option, buf);
	}
	else{
		ret = option_subtitle(option, "none");
	}

	if (!ret){
		log_error("option_subtitle() failed");
	}

	free(buf);
	return ret;
}

static void free_option_subtitles(char** options, size_t len){
	size_t i;
	for (i = 0; i < len; ++i){
		free(options[i]);
	}
}
#define FREE_OPTION_SUBTITLES(x) free_option_subtitles(x, ARRAY_SIZE(x))

static void invalid_option(int chosen, int array_size){
	char msg[128];
	const char* choices[] = {
		"OK"
	};
	sprintf(msg, "Option %d chosen of %d. This should never happen.", chosen, array_size - 1);
	display_dialog(choices, ARRAY_SIZE(choices), msg);
}

int menu_compression_level(struct options* opt){
	int res;
	const char* options_compression_level[] = {
		"Default",
		"1 (fastest, lowest compression)",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9 (slowest, highest compression)"
	};

	res = display_menu(options_compression_level, ARRAY_SIZE(options_compression_level), "Select a compression level");
	opt->c_level = res;
	return 0;
}

int menu_compressor(struct options* opt){
	int res;
	const char* options_compressor[] = {
		"gzip  (default)",
		"bzip2 (higher compression, slower)",
		"xz    (highest compression, slowest)",
		"lz4   (fastest, lowest compression)",
		"none",
		"Exit"
	};
	enum compressor list_compressor[] = {
		COMPRESSOR_GZIP,
		COMPRESSOR_BZIP2,
		COMPRESSOR_XZ,
		COMPRESSOR_LZ4,
		COMPRESSOR_NONE
	};

	res = display_menu(options_compressor, ARRAY_SIZE(options_compressor), "Select a compression algorithm");
	if (res == 5){
		return 0;
	}
	opt->c_type = list_compressor[res];
	return 0;
}

int menu_checksum(struct options* opt){
	int res;
	const char* options_checksum[] = {
		"sha1   (default)",
		"sha256 (less collisions, slower)",
		"sha512 (lowest collisions, slowest)",
		"md5    (fastest, most collisions)",
		"none",
		"Exit"
	};
	const EVP_MD*(*list_checksum[])(void) = {
		EVP_sha1,
		EVP_sha256,
		EVP_sha512,
		EVP_md5,
		EVP_md_null
	};

	res = display_menu(options_checksum, ARRAY_SIZE(options_checksum), "Select a checksum algorithm");
	if (res == 5){
		return 0;
	}
	opt->hash_algorithm = list_checksum[res] ? (list_checksum[res])() : NULL;
	return 0;
}

int menu_encryption(struct options* opt){
	int res_encryption = -1;
	int res_keysize = -1;
	int res_mode = -1;
	char* tmp = NULL;
	const char* options_encryption[] = {
		"AES (default)",
		"Camellia",
		"SEED",
		"Blowfish",
		"Triple DES (EDE3)",
		"None",
		"Exit"
	};
	const char* list_encryption[] = {
		"aes",
		"camellia",
		"seed",
		"bf",
		"des-ede3",
		NULL
	};
	const char* options_keysize[] = {
		"256 (default)",
		"192 (faster, less secure)",
		"128 (fastest, least secure)"
	};
	const char* list_keysize[] = {
		"-256",
		"-192",
		"-128"
	};
	const char* options_mode[] = {
		"Cipher Block Chaining (CBC) (default)",
		"Cipher Feedback (CFB)",
		"Output Feedback (OFB)",
		"Counter (CTR)",
	};
	const char* list_mode[] = {
		"-cbc",
		"-cfb",
		"-ofb",
		"-ctr"
	};

	res_encryption = display_menu(options_encryption, ARRAY_SIZE(options_encryption), "Select an encryption algorithm");
	if (res_encryption == 5){
		opt->enc_algorithm = NULL;
		return 0;
	}
	if (res_encryption == 6){
		return 0;
	}

	if (res_encryption <= 1){
		res_keysize = display_menu(options_keysize, ARRAY_SIZE(options_keysize), "Select a key size");
	}

	if (res_encryption <= 1){
		res_mode = display_menu(options_mode, ARRAY_SIZE(options_mode), "Select an encryption mode");
	}
	else if (res_encryption <= 4){
		res_mode = display_menu(options_mode, ARRAY_SIZE(options_mode) - 1, "Select an encryption mode");
	}

	if (res_encryption < 0 || res_encryption == 5){
		opt->enc_algorithm = NULL;
		return 0;
	}
	tmp = malloc(sizeof("camellia-256-cbc"));
	if (!tmp){
		log_enomem();
		return -1;
	}
	strcpy(tmp, list_encryption[res_encryption]);
	if (res_keysize >= 0){
		strcat(tmp, list_keysize[res_keysize]);
	}
	if (res_mode >= 0){
		strcat(tmp, list_mode[res_mode]);
	}
	opt->enc_algorithm = EVP_get_cipherbyname(tmp);
	free(tmp);

	return 0;
}

int menu_enc_password(struct options* opt){
	int res;
	char* pw;
	printf("Enter nothing to clear\n");

	while ((res = crypt_getpassword("Enter password:", "Verify password:", &pw)) > 0){
		printf("The passwords do not match");
	}

	if (res < 0){
		log_error("Failed to read password");
		return -1;
	}

	if (strcmp(pw, "") == 0){
		if (opt->enc_password){
			free(opt->enc_password);
		}
		opt->enc_password = NULL;
		crypt_freepassword(pw);
		return 0;
	}
	else{
		if (opt->enc_password){
			free(opt->enc_password);
		}
		opt->enc_password = sh_dup(pw);
		crypt_freepassword(pw);
	}

	return 0;
}

int menu_directories(struct options* opt){
	int res;
	int ret = 0;
	struct string_array* options_menu = sa_new();
	const char* options_dialog[] = {
		"OK"
	};
	char* options_confirm[] = {
		NULL,
		"Exit"
	};

	if (!options_menu){
		log_error("Failed to create options menu");
		ret = -1;
		goto cleanup;
	}

	/* TODO: refactor */
	do{
		char* str = NULL;
		int res_confirm;
		size_t i;

		sa_add(options_menu, "Add a directory");
		sa_add(options_menu, "Exit");
		for (i = 0; i < opt->directories->len; ++i){
			sa_add(options_menu, opt->directories->strings[i]);
		}

		res = display_menu((const char* const*)options_menu->strings, options_menu->len, "Directories");

		switch (res){
		case 0:
			str = readline("Enter directory:");
			if (strcmp(str, "") != 0 && sa_add(opt->directories, str) != 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Failed to add string to directory list");
				ret = -1;
				goto cleanup;
			}
			free(str);
			if ((res_confirm = sa_sanitize_directories(opt->directories)) > 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Directory specified was invalid");
			}
			else if (res_confirm < 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Warning: Failed to sanitize directory list");
			}
			break;
		case 1:
			break;
		default:
			options_confirm[0] = sh_sprintf("Remove %s", opt->directories->strings[res - 2]);
			if (!options_confirm[0]){
				log_enomem();
				ret = -1;
				goto cleanup;
			}
			res_confirm = display_menu((const char* const*)options_confirm, 2, "Removing directory");

			if (res_confirm == 0){
				if (sa_remove(opt->directories, res - 2) != 0){
					log_debug("Failed to remove_directory()");
					ret = -1;
					goto cleanup;
				}
			}

			free(options_confirm[0]);
			break;
		}
		sa_reset(options_menu);
	}while (res != 1);

cleanup:
	sa_free(options_menu);
	return ret;
}

int menu_exclude(struct options* opt){
	int res;
	int ret = 0;
	struct string_array* options_menu = sa_new();
	const char* options_dialog[] = {
		"OK"
	};
	char* options_confirm[] = {
		NULL,
		"Exit"
	};

	if (!options_menu){
		log_error("Failed to create options menu");
		ret = -1;
		goto cleanup;
	}

	/* TODO: refactor */
	do{
		char* str = NULL;
		int res_confirm;
		size_t i;

		sa_add(options_menu, "Add an exclude path");
		sa_add(options_menu, "Exit");
		for (i = 0; i < opt->exclude->len; ++i){
			sa_add(options_menu, opt->exclude->strings[i]);
		}

		res = display_menu((const char* const*)options_menu->strings, options_menu->len, "Exclude paths");

		switch (res){
		case 0:
			str = readline("Enter exclude path:");
			if (strcmp(str, "") != 0 && sa_add(opt->exclude, str) != 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Failed to add string to exclude list");
				ret = -1;
				goto cleanup;
			}
			free(str);
			if ((res_confirm = sa_sanitize_directories(opt->exclude)) > 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Exclude path specified was invalid");
			}
			else if (res_confirm < 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Warning: Failed to sanitize exclude list");
			}
			break;
		case 1:
			break;
		default:
			options_confirm[0] = sh_sprintf("Remove %s", opt->exclude->strings[res - 2]);
			if (!options_confirm[0]){
				log_enomem();
				ret = -1;
				goto cleanup;
			}
			res_confirm = display_menu((const char* const*)options_confirm, 2, "Removing exclude path");

			if (res_confirm == 0){
				if (sa_remove(opt->exclude, res - 2) != 0){
					log_debug("Failed to remove_directory()");
					ret = -1;
					goto cleanup;
				}
			}

			free(options_confirm[0]);
			break;
		}
		sa_reset(options_menu);
	}while (res != 1);

cleanup:
	sa_free(options_menu);
	return ret;
}

int menu_output_directory(struct options* opt){
	char* tmp = NULL;
	tmp = readline("Enter the output directory:");
	if (strlen(tmp) == 0){
		free(tmp);
	}
	else{
		free(opt->output_directory);
		opt->output_directory = tmp;
	}
	return 0;
}

int menu_cloud_provider(struct options* opt){
	int res;
	const char* options_cp[] = {
		"None",
		"mega.nz",
		"Exit"
	};

	res = display_menu(options_cp, ARRAY_SIZE(options_cp), "Choose a cloud provider");
	switch (res){
	case 0:
		opt->cloud_options->cp = CLOUD_NONE;
		break;
	case 1:
		opt->cloud_options->cp = CLOUD_MEGA;
		break;
	case 2:
		return 0;
	default:
		invalid_option(res, ARRAY_SIZE(options_cp));
		break;
	}
	return 0;
}

int menu_cloud_username(struct options* opt){
	char* tmp = readline("Enter username:");
	if (strcmp(tmp, "") == 0){
		co_set_username(opt->cloud_options, NULL);
	}
	else{
		co_set_username(opt->cloud_options, tmp);
	}
	free(tmp);
	return 0;
}

int menu_cloud_password(struct options* opt){
	int res;
	char* buf;

	while ((res = crypt_getpassword("Enter password:", "Verify password:", &buf)) > 0){
		printf("The passwords do not match");
	}
	if (res < 0){
		log_error("Failed to read password");
		crypt_freepassword(buf);
		return -1;
	}

	if (strcmp(buf, "") == 0){
		co_set_password(opt->cloud_options, NULL);
		crypt_freepassword(buf);
		return 0;
	}
	else if (co_set_password(opt->cloud_options, buf) != 0){
		log_debug("Failed to set password");
		crypt_freepassword(buf);
		return -1;
	}

	crypt_freepassword(buf);
	return 0;
}

int menu_cloud_main(struct options* opt){
	int res;
	char* options_cloud_main_menu[4];

	if (!opt->cloud_options && (opt->cloud_options = co_new()) == NULL){
		log_debug("co_new() failed");
		return -1;
	}

	options_cloud_main_menu[0] = option_subtitle(       "Cloud Provider", cloud_provider_to_string(opt->cloud_options->cp));
	options_cloud_main_menu[1] = option_subtitle(       "Cloud Username", opt->cloud_options->username ? opt->cloud_options->username : "none");
	options_cloud_main_menu[2] = option_subtitle_passwd("Cloud Password", opt->cloud_options->password);
	options_cloud_main_menu[3] = option_subtitle("Exit", NULL);

	do{
		res = display_menu((const char* const*)options_cloud_main_menu, ARRAY_SIZE(options_cloud_main_menu), "Cloud Main Menu");
		switch (res){
		case 0:
			menu_cloud_provider(opt);
			free(options_cloud_main_menu[0]);
			options_cloud_main_menu[0] = option_subtitle("Cloud Provider", cloud_provider_to_string(opt->cloud_options->cp));
			break;
		case 1:
			menu_cloud_username(opt);
			free(options_cloud_main_menu[1]);
			options_cloud_main_menu[1] = option_subtitle("Cloud Username", opt->cloud_options->username ? opt->cloud_options->username : "none");
			break;
		case 2:
			menu_cloud_password(opt);
			free(options_cloud_main_menu[2]);
			options_cloud_main_menu[2] = option_subtitle_passwd("Cloud Password", opt->cloud_options->password);
			break;
		case 3:
			break;
		default:
			invalid_option(res, ARRAY_SIZE(options_cloud_main_menu));
			break;
		}
	}while (res != 3);

	FREE_OPTION_SUBTITLES(options_cloud_main_menu);
	return 0;
}

int menu_compression_main(struct options* opt){
	int res;
	char* options_compression[3];
	char buf[16];

	if (opt->c_level == 0){
		strcpy(buf, "Default");
	}
	else{
		sprintf(buf, "%d", opt->c_level);
	}

	options_compression[0] = option_subtitle("Compression Algorithm", compressor_tostring(opt->c_type));
	options_compression[1] = option_subtitle("Compression Level    ", buf);
	options_compression[2] = option_subtitle("Exit", NULL);

	do{
		res = display_menu((const char* const*)options_compression, ARRAY_SIZE(options_compression), "Compression Options");
		switch (res){
		case 0:
			menu_compressor(opt);
			free(options_compression[0]);
			options_compression[0] = option_subtitle("Compression Algorithm", compressor_tostring(opt->c_type));
			break;
		case 1:
			menu_compression_level(opt);
			free(options_compression[1]);
			if (opt->c_level == 0){
				strcpy(buf, "Default");
			}
			else{
				sprintf(buf, "%d", opt->c_level);
			}
			options_compression[1] = option_subtitle("Compression Level    ", buf);
			break;
		case 2:
			break;
		default:
			invalid_option(res, ARRAY_SIZE(options_compression));
		}
	}while (res != 2);

	FREE_OPTION_SUBTITLES(options_compression);
	return 0;
}

int menu_directories_main(struct options* opt){
	int res;
	char* options_directories_main[4];
	options_directories_main[0] = option_subtitle("Backup Directories ", NULL);
	options_directories_main[1] = option_subtitle("Exclude Directories", NULL);
	options_directories_main[2] = option_subtitle("Output Directory   ", opt->output_directory);
	options_directories_main[3] = option_subtitle("Exit", NULL);

	do{
		res = display_menu((const char* const*)options_directories_main, ARRAY_SIZE(options_directories_main), "Directory Options");
		switch (res){
		case 0:
			menu_directories(opt);
			break;
		case 1:
			menu_exclude(opt);
			break;
		case 2:
			menu_output_directory(opt);
			free(options_directories_main[2]);
			options_directories_main[2] = option_subtitle("Output Directory   ", opt->output_directory);
			break;
		case 3:
			break;
		default:
			invalid_option(res, ARRAY_SIZE(options_directories_main));
			break;
		}
	}while (res != 3);

	FREE_OPTION_SUBTITLES(options_directories_main);
	return 0;
}

int menu_encryption_main(struct options* opt){
	int res;
	char* options_encryption_main[3];
	options_encryption_main[0] = option_subtitle(       "Encryption Algorithm", EVP_CIPHER_name(opt->enc_algorithm));
	options_encryption_main[1] = option_subtitle_passwd("Encryption Password ", opt->enc_password);
	options_encryption_main[2] = option_subtitle(       "Exit", NULL);

	do{
		res = display_menu((const char* const*)options_encryption_main, ARRAY_SIZE(options_encryption_main), "Encryption Options");
		switch (res){
		case 0:
			menu_encryption(opt);
			free(options_encryption_main[0]);
			options_encryption_main[0] = option_subtitle("Encryption Algorithm", EVP_CIPHER_name(opt->enc_algorithm));
			break;
		case 1:
			menu_enc_password(opt);
			free(options_encryption_main[1]);
			options_encryption_main[1] = option_subtitle_passwd("Encryption Password ", opt->enc_password);
			break;
		case 2:
			break;
		default:
			invalid_option(res, ARRAY_SIZE(options_encryption_main));
			break;
		}
	}while (res != 2);

	FREE_OPTION_SUBTITLES(options_encryption_main);
	return 0;
}

int menu_configure(struct options* opt){
	int res;
	const char* options_main_menu[] = {
		"Cloud",
		"Compression",
		"Directories",
		"Encryption",
		"Exit"
	};
	do{
		res = display_menu(options_main_menu, ARRAY_SIZE(options_main_menu), "Configure");
		switch (res){
		case 0:
			menu_cloud_main(opt);
			break;
		case 1:
			menu_compression_main(opt);
			break;
		case 2:
			menu_directories_main(opt);
			break;
		case 3:
			menu_encryption_main(opt);
			break;
		case 4:
			break;
		default:
			invalid_option(res, ARRAY_SIZE(options_main_menu));
			return 0;

		}
	}while (res != 4);
	return 0;
}

enum operation menu_operation(void){
	int res;
	const char* options_operation[] = {
		"Backup",
		"Restore",
		"Configure",
		"Exit"
	};


	res = display_menu(options_operation, ARRAY_SIZE(options_operation), "Main Menu");
	switch (res){
	case 0:
		return OP_BACKUP;
		break;
	case 1:
		return OP_RESTORE;
		break;
	case 2:
		return OP_CONFIGURE;
		break;
	case 3:
		return OP_EXIT;
		break;
	default:
		invalid_option(res, ARRAY_SIZE(options_operation));
		return OP_INVALID;
	}

	return OP_INVALID;
}
