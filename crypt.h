/*
 * File cryptography module
 * Copyright (C) 2018 Jonathan Lemos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __CRYPT_H
#define __CRYPT_H

/* list of encryption algorithms */
#include <openssl/evp.h>

/* holds data needed for encryption
 *
 * key and iv are automatically scrubbed from memory
 * once they are used. */
typedef struct crypt_keys{
	unsigned char* key;
	unsigned char* iv;
	unsigned char salt[8];
	const EVP_CIPHER* encryption;
	int key_length, iv_length;
	/* tells if functions were called in the right order
	 * otherwise we'll get cryptic hard-to-debug sigsegv's */
	int flags;
}crypt_keys;

int crypt_scrub(void* data, int len);
unsigned char crypt_randc(void);
int crypt_secure_memcmp(const void* p1, const void* p2, int len);
int crypt_getpassword(const char* prompt, const char* verify_prompt, char* out, int out_len);
int crypt_set_encryption(const char* encryption, crypt_keys* fk);
int crypt_gen_salt(crypt_keys* fk);
int crypt_set_salt(unsigned char salt[8], crypt_keys* fk);
int crypt_gen_keys(
		unsigned char* data,
		int data_len,
		const EVP_MD* md,
		int iterations,
		crypt_keys* fk
		);
int crypt_encrypt(FILE* fp_in, crypt_keys* fk, FILE* fp_out);
int crypt_encrypt_ex(FILE* fp_in, crypt_keys* fk, FILE* fp_out, int verbose, const char* progress_msg);
int crypt_decrypt(FILE* fp_in, crypt_keys* fk, FILE* fp_out);
int crypt_decrypt_ex(FILE* fp_in, crypt_keys* fk, FILE* fp_out, int verbose, const char* progress_msg);
int crypt_extract_salt(FILE* fp_in, crypt_keys* fk);
int crypt_free(crypt_keys* fk);

#endif
