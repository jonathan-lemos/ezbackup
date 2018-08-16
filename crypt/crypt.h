/** @file crypt/crypt.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CRYPT_CRYPT_H
#define __CRYPT_CRYPT_H

#include "../attribute.h"
#include <openssl/evp.h>

/**
 * @brief Holds encryption keys and data.
 */
struct crypt_keys;

/**
 * @brief Generates a new crypt keys structure.
 *
 * @return A blank crypt keys structure, or NULL on failure.<br>
 * All values are set to 0 if it succeeds.
 */
struct crypt_keys* crypt_new(void) EZB_MALLOC_LIKE;

/**
 * @brief Scrubs data with random values.<br>
 * This prevents fragments of sensitive information such as a password or a key being left in memory.
 *
 * @param data The data to scrub.
 *
 * @param len The length of the data to scrub.
 *
 * @return 0 if it succeeds, positive if low-grade random data was used, negative if it completely failed.
 */
int crypt_scrub(void* data, int len);

/**
 * @brief Generates random data.<br>
 * This is identical to crypt_scrub(), as both functions simply fill data with random values.
 * @see crypt_scrub()
 *
 * @param data The data to fill.
 *
 * @param len The length of the data to fill.
 *
 * @return 0 if it succeeds, positive if low-grade random data was used, negative if it completely failed.
 */
#define gen_csrand(data, len) ((crypt_scrub(data, len)))

/**
 * @brief Generates a random character.
 *
 * @return A random character.
 */
unsigned char crypt_randc(void);

/**
 * @brief Turns an encryption name into its EVP_CIPHER* equivalent (e.g. "AES-256-CBC" -> EVP_aes_256_cbc())
 *
 * @param encryption_name The name of the encryption algorithm to use.
 *
 * @return Its equivalent EVP_CIPHER*, or NULL if encryption_name is NULL/invalid.
 */
const EVP_CIPHER* crypt_get_cipher(const char* encryption_name);

/**
 * @brief Sets the encryption type.<br>
 * This function must be called after crypt_new()
 *
 * @param encryption The encryption type to set.
 * @see crypt_get_cipher()
 *
 * @param fk A crypt keys structure returned by crypt_new().
 * @see crypt_new()
 *
 * @return 0 on success, negative on failure.
 */
int crypt_set_encryption(const EVP_CIPHER* encryption, struct crypt_keys* fk);

/**
 * @brief Generates a random salt.
 *
 * @param fk A crypt keys structure returned by crypt_new()
 * @see crypt_new()
 *
 * @return 0 on success, positive if low-grade random data was used, negative on failure.
 */
int crypt_gen_salt(struct crypt_keys* fk);

/**
 * @brief Sets the salt to a user-specified value.<br>
 * Do not use with a hard-coded value unless its for testing purposes, as this defeats the purpose of a salt.
 *
 * @param salt The salt to set.
 *
 * @param fk A crypt keys structure returned by crypt_new()
 * @see crypt_new()
 *
 * @return 0
 */
int crypt_set_salt(const unsigned char salt[8], struct crypt_keys* fk);

/**
 * @brief Generates encryption keys based on a password.<br>
 * This function must be called after crypt_set_encryption().<br>
 * It is highly recommended that this function is called after crypt_gen_salt() or crypt_set_salt(), as this prevents rainbow table attacks against the encrypted file.<br>
 * Otherwise, the default salt value is all zeroes.
 * @see crypt_set_encryption()
 * @see crypt_gen_salt()
 * @see crypt_set_salt()
 *
 * @param data The data to be used as a password.<br>
 * This does not have to be a string; it can be any type of data of any length.
 *
 * @param data_len The length of the data to be used as the password.
 *
 * @param md The digest algorithm to be used during password generation.<br>
 * This parameter can be NULL, in which case SHA256 is used for compatibillity with the openssl command line utility.
 *
 * @param iterations The amount of iterations the password generation utility runs through.<br>
 * This must be 1 if compatibillity with the openssl command line utility is required.
 *
 * @param fk A crypt keys structure with its encryption set.
 * @see crypt_set_encryption()
 *
 * @return 0 on success, or negative on failure.
 */
int crypt_gen_keys(const void* data, int data_len, const EVP_MD* md, int iterations, struct crypt_keys* fk);

/**
 * @brief Encrypts a file using a crypt keys structure.<br>
 * This function must be called after crypt_gen_keys().<br>
 * @see crypt_gen_keys()
 *
 * @param in Path to a file to be encrypted.
 *
 * @param fk The crypt keys structure to encrypt with.
 *
 * @param out Path to write the encrypted file to.<br>
 * If this file already exists, it will be overwritten.<br>
 * If this function fails, the output file is removed.
 *
 * @return 0 on success, or negative on failure.
 */
int crypt_encrypt(const char* in, struct crypt_keys* fk, const char* out);

/**
 * @brief Encrypts a file using a crypt keys structure and an optional progress bar.<br>
 * This function must be called after crypt_gen_keys().<br>
 * @see crypt_gen_keys()
 *
 * @param in Path to a file to be encrypted.
 *
 * @param fk The crypt keys structure to encrypt with.
 *
 * @param out Path to write the encrypted file to.<br>
 * If this file already exists, it will be overwritten.<br>
 * If this function fails, the output file is removed.
 *
 * @param verbose 0 if a progress bar should not be displayed. Any other value if it should.
 *
 * @param progress_msg The progress message to display.<br>
 * If this parameter is NULL, a default "Encrypting file..." message is displayed.
 *
 * @return 0 on success, or negative on failure.
 */
int crypt_encrypt_ex(const char* in, struct crypt_keys* fk, const char* out, int verbose, const char* progress_msg);

/**
 * @brief Decrypts a file using a crypt keys structure.<br>
 * This function must be called after crypt_gen_keys() and crypt_extract_salt()<br>
 * @see crypt_gen_keys()
 * @see crypt_extract_salt()
 *
 * @param in Path to a file to be decrypted.
 *
 * @param fk The crypt keys structure to decrypt with.
 *
 * @param out Path to write the decrypted file to.<br>
 * If this file already exists, it will be overwritten.<br>
 * If this function fails, the output file is removed.
 *
 * @return 0 on success, or negative on failure.
 */
int crypt_decrypt(const char* in, struct crypt_keys* fk, const char* out);

/**
 * @brief Decrypts a file using a crypt keys structure and an optional progress bar.<br>
 * This function must be called after crypt_gen_keys() and crypt_extract_salt()
 * @see crypt_gen_keys()
 * @see crypt_extract_salt()
 *
 * @param in Path to a file to be decrypted.
 *
 * @param fk The crypt keys structure to decrypt with.
 *
 * @param out Path to write the decrypted file to.<br>
 * If this file already exists, it will be overwritten.<br>
 * If this function fails, the output file is removed.
 *
 * @param verbose 0 if a progress bar should not be displayed. Any other value if it should.
 *
 * @param progress_msg The progress message to display.<br>
 * If this parameter is NULL, a default "Decrypting file..." message is displayed.
 *
 * @return 0 on success, or negative on failure.
 */
int crypt_decrypt_ex(const char* in, struct crypt_keys* fk, const char* out, int verbose, const char* progress_msg);

/**
 * @brief Extracts the salt from an encrypted file.
 *
 * @param in Path to a file to extract salt from.
 *
 * @param fk A crypt keys structure returned by crypt_new()
 * @see crypt_new()
 *
 * @return 0 on success, or negative on failure.
 */
int crypt_extract_salt(const char* in, struct crypt_keys* fk);

/**
 * @brief Frees all memory associated with a crypt keys structure.<br>
 * This also scrubs sensitive data like encryption keys and the initialization vector.
 *
 * @param fk The crypt keys structure to free.
 *
 * @return void
 */
void crypt_free(struct crypt_keys* fk);

/**
 * @brief Resets a crypt keys structure for reuse.<br>
 * This also scrubs sensitive data like encryption keys and the initialization vector.<br>
 * The resulting crypt keys structure will be identical to one returned by crypt_new()
 * @see crypt_new()
 *
 * @param fk The crypt keys structure to reset.
 *
 * @return void
 */
void crypt_reset(struct crypt_keys* fk);

#endif
