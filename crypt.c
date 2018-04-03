/* crypt.c -- file encryption/decryption
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* prototypes */
#include "crypt.h"
/* read_file() */
#include "readfile.h"
/* handling errors */
#include "error.h"
#include <errno.h>
#include <openssl/err.h>
/* progress bar */
#include "progressbar.h"
/* swiggity swass get pass */
#include <termios.h>
/* file size */
#include <sys/stat.h>
/* the special sauce */
#include <openssl/evp.h>
/* random numbers to generate salt and gen_csrand memory */
#include <openssl/rand.h>
/* malloc() */
#include <stdlib.h>
/* read from /dev/urandom if not enough entropy */
#include <stdio.h>
#include <unistd.h>
/* memcmp */
#include <string.h>

#define FLAG_UNINITIALIZED (0x0)
#define FLAG_ENCRYPTION_SET (0x1)
#define FLAG_KEYS_SET (0x2)
#define FLAG_SALT_EXTRACTED (0x4)

/* generates cryptographically-secure random data and
 * writes it to data, or writes low-grade random data on error
 *
 * returns 0 on success, err on error. */
int crypt_scrub(void* data, int len){
	int res;

	/* checking if data is not NULL */
	if (!data){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	res = RAND_bytes(data, len);
	if (res != 1){
		/* generate low grade non-cs-secure random numbers
		 * this is so we can return something in case
		 * RAND_bytes() fails
		 *
		 * in case we cannot open /dev/urandom, abort
		 *
		 * DO NOT TRUST FOR CRYPTOGRAPHIC APPLICATIONS */
		FILE* fp;
		int i;
		volatile unsigned char* data_vol = data;

		fp = fopen("/dev/urandom", "rb");
		if (!fp){
			log_error(__FL__, "Could not generate cryptographically secure numbers, and could not open /dev/urandom for reading (%s)", strerror(errno));
			return -1;
		}

		for (i = 0; i < len; ++i){
			data_vol[i] = fgetc(fp);
		}
		fclose(fp);
		return 1;
	}
	return 0;
}
/* crypt_scrub() is really just a csrand generator */
#define gen_csrand(data, len) ((crypt_scrub(data, len)))

unsigned char crypt_randc(void){
	unsigned char ret;
	int res;

	res = RAND_bytes(&ret, 1);
	if (res != 1){
		FILE* fp = fopen("/dev/urandom", "rb");
		if (!fp){
			return random() % 256;
		}
		ret = fgetc(fp);
		fclose(fp);
		return ret;
	}
	return ret;
}

static int crypt_hashpassword(unsigned char* data, int data_len, unsigned char** salt, int* salt_len, unsigned char** hash, int* hash_len){
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
	int key_len;
	int iv_len;
	int ret;

	if (!salt_len || !salt || !data || !hash || !hash_len){
		log_error(__FL__, STR_ENULL);
		return -1;
	}
	/* generate salt if it is NULL */
	if (!(*salt)){
		*salt_len = 64;
		*salt = malloc(*salt_len);
		if (!salt){
			log_fatal(__FL__, STR_ENOMEM);
			return -1;
		}
		if ((ret = gen_csrand(*salt, *salt_len)) != 0){
			log_debug(__FL__, "Failed to generate salt with gen_csrand()");
			return ret;
		}
	}

	key_len = EVP_CIPHER_key_length(keytype());
	iv_len = EVP_CIPHER_iv_length(keytype());
	*hash_len = key_len + iv_len;

	*hash = malloc(*hash_len);
	if (!(*hash)){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}

	if (!EVP_BytesToKey(keytype(), hashtype(), *salt, data, data_len, 1000, *hash, *hash + key_len)){
		log_error(__FL__, "Failed to generate keys from data");
		ERR_print_errors_fp(stderr);
		return -1;
	}

	return 0;
}

int crypt_secure_memcmp(const void* p1, const void* p2, int len){
	volatile const unsigned char* ptr1 = p1;
	volatile const unsigned char* ptr2 = p2;
	int i;
	for (i = 0; i < len; ++i){
		if (ptr1[i] != ptr2[i]){
			return ptr1[i] - ptr2[i];
		}
	}
	return 0;
}

int crypt_getpassword(const char* prompt, const char* verify_prompt, char* out, int out_len){
	struct termios old, new;
	unsigned char* hash1 = NULL, *hash2 = NULL;
	unsigned char* salt = NULL;
	int hash_len, salt_len;
	int ret = 0;

	/* store old terminal information */
	if (tcgetattr(fileno(stdin), &old) != 0){
		log_warning(__FL__, "Failed to store current terminal settings");
	}

	/* turn off echo on terminal */
	new = old;
	new.c_lflag &= ~ECHO;
	if (tcsetattr(fileno(stdin), TCSAFLUSH, &new) != 0){
		log_warning(__FL__, "Failed to turn off terminal echo");
	}

	/* get password */
	printf("%s:", prompt);
	fgets(out, out_len - 15, stdin);
	printf("\n");
	/* remove \n */
	out[strcspn(out, "\r\n")] = '\0';
	/* if no verify prompt is specified, we can just return now */
	if (!verify_prompt){
		log_debug(__FL__, "Returning now since no verify_prompt specified");
		ret = 0;
		goto cleanup;
	}
	/* it's bad to store the password itself in memory,
	 * so we store a hash instead. */
	if ((ret = crypt_hashpassword((unsigned char*)out, strlen(out), &salt, &salt_len, &hash1, &hash_len)) != 0){
		log_error(__FL__, "Failed to hash password input");
		ret = -1;
		goto cleanup;
	}
	/* scrub the old password out of memory, so it can't be
	 * captured anymore by an attacker */
	crypt_scrub(out, strlen(out) + 5 + crypt_randc() % 11);
	log_info(__FL__, "Password should be out of memory now");

	/* verify the password */
	printf("%s:", verify_prompt);
	fgets(out, out_len - 15, stdin);
	printf("\n");
	/* remove \n */
	out[strcspn(out, "\r\n")] = '\0';
	/* same old deal */
	if ((ret = crypt_hashpassword((unsigned char*)out, strlen(out), &salt, &salt_len, &hash2, &hash_len)) != 0){
		log_error(__FL__, "Failed to hash verify password input");
		ret = -1;
		goto cleanup;
	}
	/* verify that the hashes match */
	if (crypt_secure_memcmp(hash1, hash2, hash_len) != 0){
		log_info(__FL__, "The password hashes do not match");
		/* scrub the password if they don't */
		crypt_scrub(out, out_len);
		ret = 1;
		goto cleanup;
	}

cleanup:
	free(salt);
	free(hash1);
	free(hash2);
	/* restore echo on terminal */
	if (tcsetattr(fileno(stdin), TCSAFLUSH, &old) != 0){
		log_warning(__FL__, "Failed to restore terminal echo");
	}
	return ret;
}

/* generates a random salt
 * returns 0 if salt is csrand, err if not */
int crypt_gen_salt(struct crypt_keys* fk){
	if (!fk){
		log_error(__FL__, STR_ENULL);
		return -1;
	}
	return gen_csrand(fk->salt, sizeof(fk->salt));
}

/* sets a user-defined salt
 * always returns 0 */
int crypt_set_salt(const unsigned char salt[8], struct crypt_keys* fk){
	unsigned i;

	if (!fk){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	if (salt){
		/* memcpy leaves data in memory
		 * bad for crypto applications */
		for (i = 0; i < sizeof(fk->salt); ++i){
			fk->salt[i] = salt[i];
		}
	}
	/* if salt is NULL, set salt equal to all zeroes */
	else{
		for (i = 0; i < sizeof(fk->salt); ++i){
			fk->salt[i] = '\0';
		}
	}
	return 0;
}

/* sets encryption type, this must be the first function called
 * returns 0 on success or 1 if cipher is not recognized */
int crypt_set_encryption(const char* encryption, struct crypt_keys* fk){
	if (!fk){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	fk->flags = FLAG_UNINITIALIZED;

	if (encryption){
		/* allows EVP_get_cipherbyname to work */
		OpenSSL_add_all_algorithms();
		fk->encryption = EVP_get_cipherbyname(encryption);
		if (!fk->encryption){
			log_error(__FL__, "Could not set encryption type");
			ERR_print_errors_fp(stderr);
			return -1;
		}
	}
	/* if encryption is NULL, use no encryption */
	else{
		fk->encryption = EVP_enc_null();
	}

	fk->flags = FLAG_ENCRYPTION_SET;
	return 0;
}

/* generates a key and iv based on data
 * returns 0 on success or err on error */
int crypt_gen_keys(const unsigned char* data, int data_len, const EVP_MD* md, int iterations, struct crypt_keys* fk){

	if (!fk || !data){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	/* if md is NULL, default to sha256 */
	if (!md){
		md = EVP_sha256();
	}

	/* checking if encryption is set */
	if (!(fk->flags & FLAG_ENCRYPTION_SET)){
		log_error(__FL__, "Encryption type was not set (call crypt_set_encryption())");
		return -1;
	}

	/* determining key and iv length */
	fk->key_length = EVP_CIPHER_key_length(fk->encryption);
	fk->iv_length = EVP_CIPHER_iv_length(fk->encryption);

	/* allocating space for key and iv */
	fk->key = malloc(fk->key_length);
	fk->iv = malloc(fk->iv_length);
	if (!fk->key || !fk->iv){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}

	/* generating the key and iv */
	if (!EVP_BytesToKey(fk->encryption, md, fk->salt, data, data_len, iterations, fk->key, fk->iv)){
		log_error(__FL__, "Failed to generate keys from data");
		ERR_print_errors_fp(stderr);
		return -1;
	}

	fk->flags |= FLAG_KEYS_SET;
	return 0;
}

/* frees memory malloc'd by crypt_gen_keys()
 * returns 0 if fk is not NULL */
int crypt_free(struct crypt_keys* fk){
	if (!fk){
		log_debug(__FL__, "crypt_free() arg was NULL");
		return -1;
	}

	/* scrub keys so they can't be retrieved from memory */
	crypt_scrub(fk->key, fk->key_length);
	crypt_scrub(fk->iv, fk->iv_length);

	free(fk->key);
	free(fk->iv);

	return 0;
}

/* encrypts the file
 * returns 0 on success or err on error */
int crypt_encrypt_ex(const char* in, struct crypt_keys* fk, const char* out, int verbose, const char* progress_msg){
	/* do not want null terminator */
	const char salt_prefix[8] = { 'S', 'a', 'l', 't', 'e', 'd', '_', '_'};
	EVP_CIPHER_CTX* ctx = NULL;
	unsigned char inbuffer[BUFFER_LEN];
	unsigned char* outbuffer = NULL;
	FILE* fp_in = NULL;
	FILE* fp_out = NULL;
	int inlen;
	int outlen;
	int ret = 0;
	struct progress* p = NULL;

	/* checking null arguments */
	if (!in || !fk || !out){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	fp_in = fopen(in, "rb");
	if (!fp_in){
		log_error(__FL__, STR_EFOPEN);
		return -1;
	}

	fp_out = fopen(out, "wb");
	if (!fp_out){
		log_error(__FL__, STR_EFOPEN);
		fclose(fp_in);
		return -1;
	}

	/* checking if keys were actually generated */
	if (!(fk->flags & FLAG_KEYS_SET)){
		log_error(__FL__, "Encryption keys were not generated (call crypt_gen_keys())");
		return -1;
	}

	/* write the salt prefix + salt to the file */
	fwrite(salt_prefix, 1, sizeof(salt_prefix), fp_out);
	if (ferror(fp_out)){
		log_error(__FL__, "There was an error writing to the output file");
		ret = -1;
		goto cleanup;
	}
	fwrite(fk->salt, 1, 8, fp_out);

	/* preparing progress bar */
	if (verbose){
		struct stat st;
		int fd;

		fd = fileno(fp_in);
		fstat(fd, &st);

		p = start_progress(progress_msg, st.st_size);
	}

	/* allocating space for outbuffer */

	/* encrypted data is usually longer than input data,
	 * so we must use malloc instead of just making another
	 * buffer[BUFFER_LEN] */
	outbuffer = malloc(BUFFER_LEN + EVP_CIPHER_block_size(fk->encryption));
	if (!outbuffer){
		log_fatal(__FL__, STR_ENOMEM);
		ret = -1;
		goto cleanup;
	}

	/* initializing encryption thingy */
	ctx = EVP_CIPHER_CTX_new();
	if (!ctx){
		log_error(__FL__, "Failed to initialize EVP_CIPHER_CTX");
		ERR_print_errors_fp(stderr);
		goto cleanup;
	}
	if (EVP_EncryptInit_ex(ctx, fk->encryption, NULL, fk->key, fk->iv) != 1){
		log_error(__FL__, "Failed to initialize encryption");
		ERR_print_errors_fp(stderr);
		ret = -1;
		goto cleanup;
	}

	/* while there is still data to be read in the file */
	while ((inlen = read_file(fp_in, inbuffer, sizeof(inbuffer))) > 0){
		/* encrypt it */
		if (EVP_EncryptUpdate(ctx, outbuffer, &outlen, inbuffer, inlen) != 1){
			log_error(__FL__, "Failed to encrypt data completely");
			ERR_print_errors_fp(stderr);
			ret = -1;
			goto cleanup;
		}
		/* write it to the out file */
		fwrite(outbuffer, 1, outlen, fp_out);
		if (ferror(fp_out)){
			log_error(__FL__, STR_EFWRITE, "file");
			ret = -1;
			goto cleanup;
		}
		if (verbose){
			inc_progress(p, inlen);
		}
	}
	finish_progress(p);

	/* write any padding data to the file */
	if (EVP_EncryptFinal_ex(ctx, outbuffer, &outlen) != 1){
		log_error(__FL__, "Failed to write padding data to file");
		ERR_print_errors_fp(stderr);
		ret = -1;
		goto cleanup;
	}
	fwrite(outbuffer, 1, outlen, fp_out);

cleanup:
	fclose(fp_in);
	fclose(fp_out);
	if (ctx){
		EVP_CIPHER_CTX_free(ctx);
	}
	free(outbuffer);
	return ret;
}

/* simpler wrapper for crypt_encrypt_ex if progressbar is not needed */
int crypt_encrypt(const char* in, struct crypt_keys* fk, const char* fp_out){
	return crypt_encrypt_ex(in, fk, fp_out, 0, NULL);
}

/* extracts salt from encrypted file */
int crypt_extract_salt(const char* in, struct crypt_keys* fk){
	const char salt_prefix[8] = { 'S', 'a', 'l', 't', 'e', 'd', '_', '_' };
	char salt_buffer[8];
	char buffer[8];
	FILE* fp_in = NULL;
	unsigned i;

	/* checking null arguments */
	if (!in || !fk){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	fp_in = fopen(in, "rb");
	if (!fp_in){
		log_error(__FL__, STR_EFOPEN);
		return -1;
	}

	/* check that fread works properly. also advances the file
	 * pointer to the beginning of the salt */
	if (fread(salt_buffer, 1, sizeof(salt_prefix), fp_in) != sizeof(salt_prefix)){
		log_error(__FL__, "Failed to read salt prefix from file");
		fclose(fp_in);
		return -1;
	}

	/* read the salt into the buffer, check if the correct
	 * amount of bytes were read */
	if (fread(buffer, 1, sizeof(buffer), fp_in) != sizeof(buffer)){
		log_error(__FL__, "Failed to read salt from file");
		fclose(fp_in);
		return -1;
	}

	/* memcpy() leaves data in memory
	 * bad for crypto applications */
	for (i = 0; i < sizeof(fk->salt); ++i){
		fk->salt[i] = buffer[i];
	}

	fk->flags |= FLAG_SALT_EXTRACTED;
	fclose(fp_in);
	return 0;
}

/* decrypts the file
 * returns 0 on success or err on error */
int crypt_decrypt_ex(const char* in, struct crypt_keys* fk, const char* out, int verbose, const char* progress_msg){
	/* do not want null terminator */
	EVP_CIPHER_CTX* ctx = NULL;
	unsigned char inbuffer[BUFFER_LEN];
	unsigned char* outbuffer = NULL;
	FILE* fp_in = NULL;
	FILE* fp_out = NULL;
	int inlen;
	int outlen;
	int ret = 0;
	struct progress* p = NULL;

	/* checking null arguments */
	if (!in || !fk || !out){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	fp_in = fopen(in, "rb");
	if (!fp_in){
		log_error(__FL__, STR_EFOPEN);
		return -1;
	}

	fp_out = fopen(out, "wb");
	if (!fp_out){
		log_error(__FL__, STR_EFOPEN);
		fclose(fp_in);
		return -1;
	}

	/* checking if keys were actually generated */
	if (!(fk->flags & FLAG_KEYS_SET)){
		log_error(__FL__, "Decryption keys were not generated (call crypt_gen_keys())");
		return -1;
	}

	/* checking if salt was extracted */
	if (!(fk->flags & FLAG_SALT_EXTRACTED)){
		log_error(__FL__, "Salt was not extracted from the file (call crypt_extract_salt())");
		return -1;
	}

	/* isn't the decrypted length supposed to be lower than
	 * the encrypted length?
	 *
	 * apparently not. that's why we have to do this malloc.
	 */
	outbuffer = malloc(BUFFER_LEN + EVP_CIPHER_block_size(fk->encryption));
	if (!outbuffer){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}

	/* preparing progress bar */
	if (verbose){
		struct stat st;
		int fd;

		fd = fileno(fp_in);
		fstat(fd, &st);

		p = start_progress(progress_msg, st.st_size);
	}

	/* no need for malloc() like above, since the decrypted
	 * data is shorter than encrypted. */

	/* initializing cipher context */
	ctx = EVP_CIPHER_CTX_new();
	if (!ctx){
		log_error(__FL__, "Failed to initialize EVP_CIPHER_CTX");
		ERR_print_errors_fp(stderr);
		goto cleanup;
	}
	if (EVP_DecryptInit_ex(ctx, fk->encryption, NULL, fk->key, fk->iv) != 1){
		log_error(__FL__, "Failed to intitialize decryption process");
		ERR_print_errors_fp(stderr);
		ret = -1;
		goto cleanup;
	}

	/* advance file pointer beyond salt */
	fseek(fp_in, 8 + sizeof(fk->salt), SEEK_SET);

	/* while there is data in the input file */
	while ((inlen = read_file(fp_in, inbuffer, sizeof(inbuffer))) > 0){
		/* decrypt it */
		if (EVP_DecryptUpdate(ctx, outbuffer, &outlen, inbuffer, inlen) != 1){
			log_error(__FL__, "Failed to completely decrypt file");
			ERR_print_errors_fp(stderr);
			ret = -1;
			goto cleanup;
		}
		/* write it to the output file */
		fwrite(outbuffer, 1, outlen, fp_out);
		if (verbose){
			inc_progress(p, inlen);
		}
	}
	finish_progress(p);

	/* not sure what needs to be finalized, but ehh */
	if (EVP_DecryptFinal_ex(ctx, outbuffer, &outlen) != 1){
		log_error(__FL__, "Failed to write padding data to file");
		ERR_print_errors_fp(stderr);
		ret = -1;
		goto cleanup;
	}
	fwrite(outbuffer, 1, outlen, fp_out);

cleanup:
	fclose(fp_in);
	fclose(fp_out);
	free(outbuffer);
	if (ctx){
		EVP_CIPHER_CTX_free(ctx);
	}
	return ret;
}

int crypt_decrypt(const char* in, struct crypt_keys* fk, const char* fp_out){
	return crypt_decrypt_ex(in, fk, fp_out, 0, NULL);
}
