/* crypt.h -- file encryption/decryption
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CRYPT_H
#define __CRYPT_H

/* list of encryption algorithms */
#include <openssl/evp.h>

/* holds data needed for encryption
 *
 * key and iv are automatically scrubbed from memory
 * once they are used. */
struct crypt_keys{
	unsigned char* key;
	unsigned char* iv;
	unsigned char salt[8];
	const EVP_CIPHER* encryption;
	int key_length, iv_length;
	/* tells if functions were called in the right order
	 * otherwise we'll get cryptic hard-to-debug sigsegv's */
	int flags;
};

int crypt_scrub(void* data, int len);
unsigned char crypt_randc(void);
int crypt_secure_memcmp(const void* p1, const void* p2, int len);
int crypt_getpassword(const char* prompt, const char* verify_prompt, char* out, int out_len);
int crypt_set_encryption(const char* encryption, struct crypt_keys* fk);
int crypt_gen_salt(struct crypt_keys* fk);
int crypt_set_salt(unsigned char salt[8], struct crypt_keys* fk);
int crypt_gen_keys(unsigned char* data, int data_len, const EVP_MD* md, int iterations, struct crypt_keys* fk);
int crypt_encrypt(FILE* fp_in, struct crypt_keys* fk, FILE* fp_out);
int crypt_encrypt_ex(FILE* fp_in, struct crypt_keys* fk, FILE* fp_out, int verbose, const char* progress_msg);
int crypt_decrypt(FILE* fp_in, struct crypt_keys* fk, FILE* fp_out);
int crypt_decrypt_ex(FILE* fp_in, struct crypt_keys* fk, FILE* fp_out, int verbose, const char* progress_msg);
int crypt_extract_salt(FILE* fp_in, struct crypt_keys* fk);
int crypt_free(struct crypt_keys* fk);

#endif
