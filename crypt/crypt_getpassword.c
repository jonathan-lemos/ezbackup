/* crypt_getpassword.c -- reads password securely from terminal
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "crypt.h"
#include "../error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <openssl/err.h>

int terminal_set_echo(int enable){
	struct termios term_info;

	if (enable != 0 && enable != 1){
		log_einval(enable);
		return -1;
	}

	/* get current terminal information */
	if (tcgetattr(fileno(stdin), &term_info) != 0){
		log_warning_ex("Failed to fetch current terminal settings (%s)", strerror(errno));
		return -1;
	}

	/* set ECHO bit to 0/1 depending on argument */
	term_info.c_lflag = (term_info.c_lflag & ~(ECHO)) | (enable * ECHO);

	/* set new terminal settings */
	if (tcsetattr(fileno(stdin), TCSAFLUSH, &term_info) != 0){
		log_warning_ex("Failed to set terminal echo (%s)", strerror(errno));
		return -1;
	}

	return 0;
}

char* get_stdin_secure(const char* prompt){
	char input_buffer[256];
	char* str = NULL;
	int str_len = 1;

	return_ifnull(prompt, NULL);

	printf("%s", prompt);

	str = calloc(1, 1);
	if (!str){
		log_enomem();
		return NULL;
	}

	do{
		char* tmp;
		/* read sizeof(input_buffer) bytes from stdin */
		fgets(input_buffer, sizeof(input_buffer), stdin);
		input_buffer[strcspn(input_buffer, "\r\n")] = '\0';

		/* make room for new data */
		str_len += strlen(input_buffer);
		/* do not use realloc()
		 * if the memory block moves, it leaves traces
		 * of old password in memory */
		tmp = malloc(str_len);
		if (!tmp){
			log_enomem();
			free(str);
			return NULL;
		}
		strcpy(tmp, str);

		/* scrub old password from memory */
		crypt_scrub(str, strlen(str));
		free(str);

		/* concat new data to tmp */
		strcat(tmp, input_buffer);

		/* finally set str to tmp */
		str = tmp;
		/* while input_buffer is full (stdin still has data) */
	}while (strlen(input_buffer) >= sizeof(input_buffer) - 1);

	return str;
}

int crypt_hashpassword(const unsigned char* data, size_t data_len, unsigned char** salt, int* salt_len, unsigned char** hash, int* hash_len){
	/* we're not actually using this to encrypt anything,
	 * we're just using it as a parameter to EVP_BytesToKey
	 * so we can generate our keypair which is irreversable
	 *
	 * AES-256-XTS has a 512-bit key vs. others which only
	 * have 256 bits. This results in a longer hash
	 * */
	const EVP_CIPHER*(*keytype)(void) = EVP_aes_256_xts;
	/* this is not the hash algorithm we are using
	 * this is just a parameter to EVP_BytesToKey
	 * which uses something like PBKDF2 to generate
	 * the keypair
	 *
	 * SHA-512 may be a little overkill, but ehh
	 * */
	const EVP_MD*(*hashtype)(void) = EVP_sha512;
	size_t key_len;
	size_t iv_len;
	int ret;

	return_ifnull(data, -1);
	return_ifnull(salt, -1);
	return_ifnull(hash, -1);
	return_ifnull(hash_len, -1);

	/* generate salt if it is NULL */
	if (!(*salt)){
		*salt_len = 64;
		*salt = malloc(*salt_len);
		if (!salt){
			log_enomem();
			return -1;
		}
		if ((ret = gen_csrand(*salt, *salt_len)) != 0){
			log_debug("Failed to generate salt with gen_csrand()");
			return ret;
		}
	}

	/* get required key lengths and malloc space for hash */
	key_len = EVP_CIPHER_key_length(keytype());
	iv_len = EVP_CIPHER_iv_length(keytype());
	*hash_len = key_len + iv_len;

	*hash = malloc(*hash_len);
	if (!(*hash)){
		log_enomem();
		return -1;
	}

	/* generate hash with 25000 rounds of the underlying pbkdf function */
	if (!EVP_BytesToKey(keytype(), hashtype(), *salt, data, data_len, 25000, *hash, *hash + key_len)){
		log_error("Failed to generate keys from data");
		ERR_print_errors_fp(stderr);
		return -1;
	}

	return 0;
}

/* regular memcmp may leave password in registers/memory
 * this function does not */
int crypt_secure_memcmp(const void* p1, const void* p2, size_t len){
	volatile const unsigned char* ptr1 = p1;
	volatile const unsigned char* ptr2 = p2;
	size_t i;

	for (i = 0; i < len; ++i){
		if (ptr1[i] != ptr2[i]){
			return ptr1[i] - ptr2[i];
		}
	}
	return 0;
}

int crypt_getpassword(const char* prompt, const char* verify_prompt, char** out){
	unsigned char* hash1 = NULL, *hash2 = NULL;
	unsigned char* salt = NULL;
	int hash_len, salt_len;
	int ret = 0;

	return_ifnull(prompt, -1);
	return_ifnull(out, -1);

	if (terminal_set_echo(0) != 0){
		log_warning("Terminal echo could not be disabled");
	}

	/* get password */
	*out = get_stdin_secure(prompt);
	if (!(*out)){
		log_error("Failed to get password from stdin");
		ret = -1;
		goto cleanup;
	}

	if (!verify_prompt){
		log_info("verify_prompt is NULL so returning now");
		ret = 0;
		goto cleanup;
	}

	/* it's bad to store the password itself in memory,
	 * so we store a hash instead. */
	if ((ret = crypt_hashpassword((unsigned char*)*out, strlen(*out), &salt, &salt_len, &hash1, &hash_len)) != 0){
		log_error("Failed to hash password input");
		ret = -1;
		goto cleanup;
	}

	/* scrub the old password out of memory, so it can't be
	 * captured anymore by an attacker */
	crypt_scrub(*out, strlen(*out));
	log_info("Password should be out of memory now");

	/* verify the password */
	*out = get_stdin_secure(verify_prompt);
	if (!(*out)){
		log_error("Failed to get password from stdin");
		ret = -1;
		goto cleanup;
	}

	/* same old deal */
	if ((ret = crypt_hashpassword((unsigned char*)*out, strlen(*out), &salt, &salt_len, &hash2, &hash_len)) != 0){
		log_error("Failed to hash verify password input");
		crypt_scrub(*out, strlen(*out));
		ret = -1;
		goto cleanup;
	}

	/* verify that the hashes match */
	if (crypt_secure_memcmp(hash1, hash2, hash_len) != 0){
		log_info("The password hashes do not match");
		crypt_scrub(*out, strlen(*out));
		ret = 1;
		goto cleanup;
	}

cleanup:
	free(salt);
	free(hash1);
	free(hash2);
	/* restore echo on terminal */
	if (terminal_set_echo(1) != 0){
		log_warning("Failed to re-enable terminal echo");
	}

	if (ret != 0){
		if (*out){
			crypt_scrub(*out, strlen(*out));
		}
		free(*out);
		*out = NULL;
	}

	return ret;
}

void crypt_erasepassword(char* password){
	if (!password){
		log_debug("password was NULL");
		return;
	}

	crypt_scrub(password, strlen(password));
	free(password);
}
