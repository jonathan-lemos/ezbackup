/* options_menu.c -- menu driver for options.c
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
#include <string.h>
#if defined(__linux__)
#include <editline/readline.h>
#elif defined(__APPLE__)
#include <readline/readline.h>
#else
#error "This operating system is not supported"
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void invalid_option(int chosen, int array_size){
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
	opt->comp_level = res;
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
	enum COMPRESSOR list_compressor[] = {
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
	opt->comp_algorithm = list_compressor[res];
	return menu_compression_level(opt);
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
		"none",
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
		opt->enc_algorithm = EVP_enc_null();
		return 0;
	}
	if (res_encryption == 6){
		return 0;
	}
	if (res_encryption <= 1){
		res_keysize = display_menu(options_keysize, ARRAY_SIZE(options_keysize), "Select a key size");
	}
	if (res_encryption <= 2){
		res_mode = display_menu(options_mode, ARRAY_SIZE(options_mode), "Select an encryption mode");
	}
	else if (res_encryption <= 4){
		res_mode = display_menu(options_mode, ARRAY_SIZE(options_mode) - 1, "Select an encryption mode");
	}

	if (res_encryption < 0 || res_encryption == 5){
		opt->enc_algorithm = EVP_enc_null();
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
	const char** options_initial = NULL;
	const char* options_dialog[] = {
		"OK"
	};

	/* TODO: refactor */
	do{
		size_t i;

		options_initial = malloc((opt->directories->len + 2) * sizeof(*(opt->directories)));
		if (!options_initial){
			log_enomem();
			return -1;
		}
		options_initial[0] = "Add a directory";
		options_initial[1] = "Exit";
		for (i = 2; i < opt->directories->len + 2; ++i){
			options_initial[i] = opt->directories->strings[i - 2];
		}

		res = display_menu(options_initial, opt->directories->len + 2, "Directories");

		switch (res){
			char* str;
			int res_confirm;
			char** options_confirm;
		case 0:
			str = readline("Enter directory:");
			if (strcmp(str, "") != 0 && sa_add(opt->directories, str) != 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Failed to add string to directory list");
				return -1;
			}
			free(str);
			if ((res = sa_sanitize_directories(opt->directories)) > 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Directory specified was invalid");
			}
			else if (res < 0){
				display_dialog(options_dialog, ARRAY_SIZE(options_dialog), "Warning: Failed to sanitize directory list");
			}
			break;
		case 1:
			break;
		default:
			options_confirm = malloc(2 * sizeof(*options_confirm));
			if (!options_confirm){
				log_enomem();
				return -1;
			}
			options_confirm[0] = malloc(sizeof("Remove ") + strlen(opt->directories->strings[res - 2]));
			if (!options_confirm){
				log_enomem();
				return -1;
			}
			strcpy(options_confirm[0], "Remove ");
			strcat(options_confirm[0], opt->directories->strings[res - 2]);
			options_confirm[1] = "Exit";
			res_confirm = display_menu((const char**)options_confirm, 2, "Removing directory");

			if (res_confirm == 0){
				if (sa_remove(opt->directories, res - 2) != 0){
					log_debug("Failed to remove_directory()");
					return -1;
				}
			}

			free(options_confirm[0]);
			free(options_confirm);
			break;
		}
	}while (res != 1);
	return 0;
}

int menu_exclude(struct options* opt){
	int res;
	const char** options_initial = NULL;
	const char* title = "Exclude Paths";

	/* TODO: refactor */
	do{
		size_t i;

		options_initial = malloc((opt->exclude->len + 2) * sizeof(*(opt->exclude)));
		if (!options_initial){
			log_enomem();
			return -1;
		}
		options_initial[0] = "Add an exclude path";
		options_initial[1] = "Exit";
		for (i = 2; i < opt->exclude->len + 2; ++i){
			options_initial[i] = opt->exclude->strings[i - 2];
		}

		res = display_menu(options_initial, opt->exclude->len + 2, title);

		switch (res){
			char* str;
			int res_confirm;
			char** options_confirm;
		case 0:
			str = readline("Enter exclude path:");
			if (strcmp(str, "") != 0 && sa_add(opt->exclude, str) != 0){
				log_debug("Failed to add string to exclude list");
				return -1;
			}
			free(str);
			if ((res = sa_sanitize_directories(opt->exclude)) > 0){
				title = "Exclude path specified was invalid";
			}
			else if (res < 0){
				log_warning("Failed to sanitize exclude list");
			}
			else{
				title = "Exclude paths";
			}
			break;
		case 1:
			break;
		default:
			options_confirm = malloc(2 * sizeof(*options_confirm));
			if (!options_confirm){
				log_enomem();
				return -1;
			}
			options_confirm[0] = malloc(sizeof("Remove ") + strlen(opt->exclude->strings[res - 2]));
			if (!options_confirm){
				log_enomem();
				return -1;
			}
			strcpy(options_confirm[0], "Remove ");
			strcat(options_confirm[0], opt->exclude->strings[res - 2]);
			options_confirm[1] = "Exit";
			res_confirm = display_menu((const char**)options_confirm, 2, "Removing exclude path");

			if (res_confirm == 0){
				if (sa_remove(opt->exclude, res - 2) != 0){
					log_debug("Failed to remove_string()");
					return -1;
				}
			}

			free(options_confirm[0]);
			free(options_confirm);
			break;
		}
	}while (res != 1);
	return 0;
}

int menu_output_directory(struct options* opt){
	char* tmp;
	if (opt->output_directory){
		printf("Old directory: %s\n", opt->output_directory);
		free(opt->output_directory);
	}
	tmp = readline("Enter the output directory:");
	if (strcmp(tmp, "") == 0){
		free(tmp);
	}
	else{
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
		free(tmp);
		co_set_username(opt->cloud_options, NULL);
	}
	return 0;
}

int menu_cloud_password(struct options* opt){
	int res;
	char* buf;
	printf("Enter nothing to clear\n");

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
	const char* options_cloud_main_menu[] = {
		"Cloud Provider",
		"Username",
		"Password",
		"Exit"
	};

	if (!opt->cloud_options && (opt->cloud_options = co_new()) == NULL){
		log_debug("co_new() failed");
		return -1;
	}

	do{
		res = display_menu(options_cloud_main_menu, ARRAY_SIZE(options_cloud_main_menu), "Cloud Main Menu");
		switch (res){
		case 0:
			menu_cloud_provider(opt);
			break;
		case 1:
			menu_cloud_username(opt);
			break;
		case 2:
			menu_cloud_password(opt);
			break;
		case 3:
			break;
		default:
			invalid_option(res, ARRAY_SIZE(options_cloud_main_menu));
			break;
		}
	}while (res != 3);

	return 0;
}

int menu_compression_main(struct options* opt){
	int res;
	const char* options_compression[] = {
		"Compression Algorithm",
		"Compression Level",
		"Exit"
	};

	do{
		res = display_menu(options_compression, ARRAY_SIZE(options_compression), "Compression Options");
		switch (res){
		case 0:
			menu_compressor(opt);
			break;
		case 1:
			menu_compression_level(opt);
			break;
		case 2:
			break;
		default:
			invalid_option(res, ARRAY_SIZE(options_compression));
		}
	}while (res != 2);

	return 0;
}

int menu_directories_main(struct options* opt){
	int res;
	const char* options_directories_main[] = {
		"Backup Directories",
		"Exclude Directories",
		"Output Directory",
		"Exit"
	};

	do{
		res = display_menu(options_directories_main, ARRAY_SIZE(options_directories_main), "Directory Options");
		switch (res){
		case 0:
			menu_directories(opt);
			break;
		case 1:
			menu_exclude(opt);
			break;
		case 2:
			menu_output_directory(opt);
			break;
		case 3:
			break;
		default:
			invalid_option(res, ARRAY_SIZE(options_directories_main));
			break;
		}
	}while (res != 3);
	return 0;
}

int menu_encryption_main(struct options* opt){
	int res;
	const char* options_encryption_main[] = {
		"Encryption Algorithm",
		"Encryption Password",
		"Exit"
	};

	do{
		res = display_menu(options_encryption_main, ARRAY_SIZE(options_encryption_main), "Encryption Options");
		switch (res){
		case 0:
			menu_encryption(opt);
			break;
		case 1:
			menu_enc_password(opt);
			break;
		case 2:
			break;
		default:
			invalid_option(res, ARRAY_SIZE(options_encryption_main));
			break;
		}
	}while (res != 2);

	return 0;
}

int menu_main_configure(struct options* opt){
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
			menu_encryption(opt);
			break;
		case 3:
			menu_exclude(opt);
			break;
		case 4:
			break;
		default:
			invalid_option(res, ARRAY_SIZE(options_main_menu));
			return 0;

		}
	}while (res != 5);
	return 0;
}

enum OPERATION menu_main(struct options* opt){
	int res;
	const char* options_operation[] = {
		"Backup",
		"Restore",
		"Configure",
		"Exit"
	};

	do{
		res = display_menu(options_operation, ARRAY_SIZE(options_operation), "Main Menu");
		switch (res){
		case 0:
			return OP_BACKUP;
			break;
		case 1:
			return OP_RESTORE;
			break;
		case 2:
			menu_main_configure(opt);
			break;
		case 3:
			return OP_EXIT;
			break;
		default:
			invalid_option(res, ARRAY_SIZE(options_operation));
			return OP_INVALID;
		}
	}while (res == 2);

	return OP_INVALID;
}
