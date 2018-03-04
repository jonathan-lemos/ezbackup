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

int crypt_scrub(unsigned char* data, int len);
unsigned char crypt_randc(void);
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
int crypt_encrypt(const char* in, crypt_keys* fk, const char* out);
int crypt_encrypt_ex(const char* in, crypt_keys* fk, const char* out, int verbose, const char* progress_msg);
int crypt_decrypt(const char* in, crypt_keys* fk, const char* out);
int crypt_decrypt_ex(const char* in, crypt_keys* fk, const char* out, int verbose, const char* progress_msg);
int crypt_extract_salt(const char* in, crypt_keys* fk);
int crypt_free(crypt_keys* fk);
char* crypt_strerror(unsigned long err);

#endif
