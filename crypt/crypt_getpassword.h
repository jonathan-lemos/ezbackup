/* crypt_getpassword.h -- reads password securely from terminal
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CRYPT_CRYPT_GETPASSWORD_H
#define __CRYPT_CRYPT_GETPASSWORD_H

int crypt_getpassword(const char* prompt, const char* verify_prompt, char** out);
void crypt_freepassword(char* password);

#ifdef __UNIT_TESTING__
int terminal_set_echo(int enable);
char* get_stdin_secure(const char* prompt);
int crypt_hashpassword(const unsigned char* data, size_t data_len, unsigned char** salt, int* salt_len, unsigned char** hash, int* hash_len);
#endif

#endif
