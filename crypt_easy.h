/* crypt_easy.h - easy encrypt/decrypt file
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CRYPT_EASY_H
#define __CRYPT_EASY_H

#include <openssl/evp.h>

int easy_encrypt(const char* in, const char* out, const EVP_CIPHER* enc_algorithm, int verbose);
int easy_decrypt(const char* in, const char* out, const EVP_CIPHER* enc_algorithm, int verbose);

#endif
