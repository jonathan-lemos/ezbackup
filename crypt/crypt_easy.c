/* crypt_easy.c - easy encrypt/decrypt file
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "crypt_easy.h"
#include "crypt.h"
#include "error.h"
#include "coredumps.h"
#include <string.h>

int easy_encrypt(const char* in, const char* out, const EVP_CIPHER* enc_algorithm, int verbose){
	char pwbuffer[1024];
	struct crypt_keys* fk;
	char prompt[128];
	int ret;

	/* disable core dumps if possible */
	if (disable_core_dumps() != 0){
		log_warning("Core dumps could not be disabled\n");
	}

	if ((fk = crypt_new()) == NULL){
		log_debug("Failed to generate new struct crypt_keys");
		return -1;
	}

	if (crypt_set_encryption(enc_algorithm, fk) != 0){
		log_debug("Could not set encryption type");
		ret = -1;
		goto cleanup;
	}

	if (crypt_gen_salt(fk) != 0){
		log_debug("Could not generate salt");
		ret = -1;
		goto cleanup;
	}

	sprintf(prompt, "Enter %s encryption password", EVP_CIPHER_name(enc_algorithm));
	/* PASSWORD IN MEMORY */
	while ((ret = crypt_getpassword(prompt,
					"Verify encryption password",
					pwbuffer,
					sizeof(pwbuffer))) > 0){
		printf("\nPasswords do not match\n");
	}
	if (ret < 0){
		log_debug("crypt_getpassword() failed");
		crypt_scrub(pwbuffer, strlen(pwbuffer));
		ret = -1;
		goto cleanup;
	}

	if ((crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, fk)) != 0){
		crypt_scrub(pwbuffer, strlen(pwbuffer));
		log_debug("crypt_gen_keys() failed");
		ret = -1;
		goto cleanup;
	}
	/* don't need to scrub entire buffer, just where the password was
	 * and a little more so attackers don't know how long the password was */
	crypt_scrub((unsigned char*)pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
	/* PASSWORD OUT OF MEMORY */

	if ((crypt_encrypt_ex(in, fk, out, verbose, "Encrypting file...")) != 0){
		crypt_free(fk);
		log_debug("crypt_encrypt() failed");
		ret = -1;
		goto cleanup;
	}

cleanup:
	/* shreds keys as well */
	crypt_free(fk);
	if (enable_core_dumps() != 0){
		log_debug("enable_core_dumps() failed");
	}
	return 0;
}

int easy_decrypt(const char* in, const char* out, const EVP_CIPHER* enc_algorithm, int verbose){
	char pwbuffer[1024];
	struct crypt_keys* fk = NULL;
	char prompt[128];
	int ret = 0;

	if (disable_core_dumps() != 0){
		log_debug("Could not disable core dumps");
	}

	if ((fk = crypt_new()) == NULL){
		log_debug("Failed to initialize crypt_keys");
		return -1;
	}

	if (crypt_set_encryption(enc_algorithm, fk) != 0){
		log_debug("crypt_set_encryption() failed");
		ret = -1;
		goto cleanup;
	}

	if (crypt_extract_salt(in, fk) != 0){
		log_debug("crypt_extract_salt() failed");
		ret = -1;
		goto cleanup;
	}

	sprintf(prompt, "Enter %s decryption password", EVP_CIPHER_name(enc_algorithm));
	if ((crypt_getpassword(prompt, NULL, pwbuffer, sizeof(pwbuffer))) != 0){
		log_debug("crypt_getpassword() failed");
		ret = -1;
		goto cleanup;
	}

	if ((crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, fk)) != 0){
		crypt_scrub(pwbuffer, strlen(pwbuffer));
		log_debug("crypt_gen_keys() failed");
		ret = -1;
		goto cleanup;
	}

	if ((crypt_decrypt_ex(in, fk, out, verbose, "Decrypting file...")) != 0){
		log_debug("crypt_decrypt() failed");
		ret = -1;
		goto cleanup;
	}

cleanup:
	fk ? crypt_free(fk) : 0;
	if (enable_core_dumps() != 0){
		log_debug("enable_core_dumps() failed");
	}
	return ret;
}
