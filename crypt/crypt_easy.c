/* crypt_easy.c - easy encrypt/decrypt file
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "crypt_easy.h"
#include "crypt.h"
#include "crypt_getpassword.h"
#include "../log.h"
#include "../coredumps.h"
#include "../filehelper.h"
#include <string.h>

int easy_encrypt(const char* in, const char* out, const EVP_CIPHER* enc_algorithm, int verbose){
	struct crypt_keys* fk = NULL;
	char prompt[128];
	char* passwd = NULL;
	int ret = 0;

	/* disable core dumps if possible */
	if (disable_core_dumps() != 0){
		log_warning("Core dumps could not be disabled\n");
	}

	if ((fk = crypt_new()) == NULL){
		log_debug("Failed to generate new struct crypt_keys");
		ret = -1;
		goto cleanup;
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

	sprintf(prompt, "Enter %s encryption password:", EVP_CIPHER_name(enc_algorithm));

	while ((ret = crypt_getpassword(prompt,
					"Verify encryption password:",
					&passwd)) > 0){
		printf("\nPasswords do not match\n");
	}
	if (ret < 0){
		log_debug("crypt_getpassword() failed");
		crypt_scrub(passwd, strlen(passwd));
		ret = -1;
		goto cleanup;
	}

	if ((crypt_gen_keys((unsigned char*)passwd, strlen(passwd), NULL, 1, fk)) != 0){
		crypt_scrub(passwd, strlen(passwd));
		log_debug("crypt_gen_keys() failed");
		ret = -1;
		goto cleanup;
	}
	/* don't need to scrub entire buffer, just where the password was */
	crypt_scrub(passwd, strlen(passwd));

	if ((crypt_encrypt_ex(in, fk, out, verbose, "Encrypting file...")) != 0){
		log_debug("crypt_encrypt() failed");
		ret = -1;
		goto cleanup;
	}

cleanup:
	/* shreds keys as well */
	fk ? crypt_free(fk) : 0;
	free(passwd);
	if (enable_core_dumps() != 0){
		log_debug("enable_core_dumps() failed");
	}
	return 0;
}

int easy_decrypt(const char* in, const char* out, const EVP_CIPHER* enc_algorithm, int verbose){
	struct crypt_keys* fk = NULL;
	char prompt[128];
	char* passwd = NULL;
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

	sprintf(prompt, "Enter %s decryption password:", EVP_CIPHER_name(enc_algorithm));
	if ((crypt_getpassword(prompt, NULL, &passwd)) != 0){
		log_debug("crypt_getpassword() failed");
		ret = -1;
		goto cleanup;
	}

	if ((crypt_gen_keys((unsigned char*)passwd, strlen(passwd), NULL, 1, fk)) != 0){
		crypt_scrub(passwd, strlen(passwd));
		log_debug("crypt_gen_keys() failed");
		ret = -1;
		goto cleanup;
	}

	crypt_scrub(passwd, strlen(passwd));

	if ((crypt_decrypt_ex(in, fk, out, verbose, "Decrypting file...")) != 0){
		log_debug("crypt_decrypt() failed");
		ret = -1;
		goto cleanup;
	}

cleanup:
	fk ? crypt_free(fk) : 0;
	free(passwd);
	if (enable_core_dumps() != 0){
		log_debug("enable_core_dumps() failed");
	}
	return ret;
}

int easy_encrypt_inplace(const char* in_out, const EVP_CIPHER* enc_algorithm, int verbose){
	struct crypt_keys* fk = NULL;
	char prompt[128];
	char* passwd = NULL;
	struct TMPFILE* tfp_tmp = NULL;
	int ret = 0;

	/* disable core dumps if possible */
	if (disable_core_dumps() != 0){
		log_warning("Core dumps could not be disabled\n");
	}

	if ((fk = crypt_new()) == NULL){
		log_debug("Failed to generate new struct crypt_keys");
		ret = -1;
		goto cleanup;
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

	sprintf(prompt, "Enter %s encryption password:", EVP_CIPHER_name(enc_algorithm));

	while ((ret = crypt_getpassword(prompt,
					"Verify encryption password:",
					&passwd)) > 0){
		printf("\nPasswords do not match\n");
	}
	if (ret < 0){
		log_debug("crypt_getpassword() failed");
		crypt_scrub(passwd, strlen(passwd));
		ret = -1;
		goto cleanup;
	}

	if ((crypt_gen_keys((unsigned char*)passwd, strlen(passwd), NULL, 1, fk)) != 0){
		crypt_scrub(passwd, strlen(passwd));
		log_debug("crypt_gen_keys() failed");
		ret = -1;
		goto cleanup;
	}
	/* don't need to scrub entire buffer, just where the password was */
	crypt_scrub(passwd, strlen(passwd));

	tfp_tmp = temp_fopen();
	if (!tfp_tmp){
		log_error("Failed to make temporary file");
		ret = -1;
		goto cleanup;
	}

	if (rename_file(in_out, tfp_tmp->name) != 0){
		log_error("Failed to move file to temporary location");
		ret = -1;
		goto cleanup;
	}

	temp_fflush(tfp_tmp);

	if ((crypt_encrypt_ex(tfp_tmp->name, fk, in_out, verbose, "Encrypting file...")) != 0){
		rename_file(tfp_tmp->name, in_out);
		log_debug("crypt_encrypt() failed");
		ret = -1;
		goto cleanup;
	}

cleanup:
	tfp_tmp ? remove(tfp_tmp->name) : 0;
	if (tfp_tmp){
		temp_fclose(tfp_tmp);
	}
	/* shreds keys as well */
	fk ? crypt_free(fk) : 0;
	free(passwd);
	if (enable_core_dumps() != 0){
		log_debug("enable_core_dumps() failed");
	}
	return 0;
}

int easy_decrypt_inplace(const char* in_out, const EVP_CIPHER* enc_algorithm, int verbose){
	struct crypt_keys* fk = NULL;
	char prompt[128];
	char* passwd = NULL;
	struct TMPFILE* tfp_tmp = NULL;
	int ret = 0;

	if (disable_core_dumps() != 0){
		log_debug("Could not disable core dumps");
	}

	if ((fk = crypt_new()) == NULL){
		log_debug("Failed to initialize crypt_keys");
		ret = -1;
		goto cleanup;
	}

	if (crypt_set_encryption(enc_algorithm, fk) != 0){
		log_debug("crypt_set_encryption() failed");
		ret = -1;
		goto cleanup;
	}

	if (crypt_extract_salt(in_out, fk) != 0){
		log_debug("crypt_extract_salt() failed");
		ret = -1;
		goto cleanup;
	}

	sprintf(prompt, "Enter %s decryption password:", EVP_CIPHER_name(enc_algorithm));
	if ((crypt_getpassword(prompt, NULL, &passwd)) != 0){
		log_debug("crypt_getpassword() failed");
		ret = -1;
		goto cleanup;
	}

	if ((crypt_gen_keys((unsigned char*)passwd, strlen(passwd), NULL, 1, fk)) != 0){
		crypt_scrub(passwd, strlen(passwd));
		log_debug("crypt_gen_keys() failed");
		ret = -1;
		goto cleanup;
	}

	crypt_scrub(passwd, strlen(passwd));

	tfp_tmp = temp_fopen();
	if (!tfp_tmp){
		log_error("Failed to generate temporary file");
		ret = -1;
		goto cleanup;
	}

	if (rename_file(in_out, tfp_tmp->name) != 0){
		log_error("Failed to move file to temporary location");
		ret = -1;
		goto cleanup;
	}

	temp_fflush(tfp_tmp);

	if ((crypt_decrypt_ex(tfp_tmp->name, fk, in_out, verbose, "Decrypting file...")) != 0){
		rename_file(tfp_tmp->name, in_out);
		log_debug("crypt_decrypt() failed");
		ret = -1;
		goto cleanup;
	}

cleanup:
	tfp_tmp ? remove(tfp_tmp->name) : 0;
	if (tfp_tmp){
		temp_fclose(tfp_tmp);
	}
	fk ? crypt_free(fk) : 0;
	free(passwd);
	if (enable_core_dumps() != 0){
		log_debug("enable_core_dumps() failed");
	}
	return ret;
}
