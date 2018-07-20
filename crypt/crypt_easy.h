/** @file crypt/crypt_easy.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CRYPT_CRYPT_EASY_H
#define __CRYPT_CRYPT_EASY_H

/**
 * @brief Encrypts a file.
 *
 * @param in Path to a file to encrypt.
 *
 * @param out Path to write the encrypted file to.<br>
 * If this function fails, the output file is removed.
 *
 * @param enc_algorithm The encryption algorithm to use (e.g. "AES-256-CBC")
 *
 * @param verbose Any value besides 0 shows a progress bar.
 *
 * @param password The password to use.<br>
 * If this is NULL, the user is asked for a password.
 *
 * @return 0 on success, or negative on failure.
 */
int easy_encrypt(const char* in, const char* out, const char* enc_algorithm, int verbose, const char* password);

/**
 * @brief Decrypts a file.
 *
 * @param in Path to a file to decrypt.
 *
 * @param out Path to write the decrypted file to.<br>
 * If this function fails, the output file is removed.
 *
 * @param enc_algorithm The decryption algorithm to use (e.g. "AES-256-CBC")
 *
 * @param verbose Any value besides 0 shows a progress bar.
 *
 * @param password The password to use.<br>
 * If this is NULL, the user is asked for a password.
 *
 * @return 0 on success, or negative on failure.
 */
int easy_decrypt(const char* in, const char* out, const char* enc_algorithm, int verbose, const char* password);

/**
 * @brief Encrypts a file in place.
 *
 * @param in_out Path to a file to encrypt.<br>
 * If this function fails, the file is unchanged.
 *
 * @param enc_algorithm The encryption algorithm to use (e.g. "AES-256-CBC")
 *
 * @param verbose Any value besides 0 shows a progress bar.
 *
 * @param password The password to use.<br>
 * If this is NULL, the user is asked for a password.
 *
 * @return 0 on success, or negative on failure.
 */
int easy_encrypt_inplace(const char* in_out, const char* enc_algorithm, int verbose, const char* password);

/**
 * @brief Decrypts a file in place.
 *
 * @param in_out Path to a file to decrypt.<br>
 * If this function fails, the file is unchanged.
 *
 * @param enc_algorithm The decryption algorithm to use (e.g. "AES-256-CBC")
 *
 * @param verbose Any value besides 0 shows a progress bar.
 *
 * @param password The password to use.<br>
 * If this is NULL, the user is asked for a password.
 *
 * @return 0 on success, or negative on failure.
 */
int easy_decrypt_inplace(const char* in_out, const char* enc_algorithm, int verbose, const char* password);

#endif
