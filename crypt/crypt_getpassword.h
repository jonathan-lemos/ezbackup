/** @file crypt/crypt_getpassword.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CRYPT_CRYPT_GETPASSWORD_H
#define __CRYPT_CRYPT_GETPASSWORD_H

/**
 * @brief Gets a password securely from the terminal.
 * Terminal echo is disabled when the password is being read.
 * Any fragments of the the user's input besides the output are erased from memory.
 *
 * @param prompt The prompt to display to the user.
 * If this is NULL, a default "Enter password:" prompt is displayed.
 *
 * @param verify_prompt The verification prompt to display to the user.
 * If this is NULL, no verification is asked for.
 *
 * @param out A pointer to a string that will contain the password that the user entered.
 * If the user's verification does not match their initial password, or the function fails, this is set to NULL.
 *
 * @return 0 on success, positive if the user's verification does not match their initial password, negative on failure.
 */
int crypt_getpassword(const char* prompt, const char* verify_prompt, char** out);

/**
 * @brief Frees a password securely.
 * This makes sure that the password does not remain in memory once it is free.
 *
 * @param password The password to free.
 *
 * @return void
 */
void crypt_freepassword(char* password);

#endif
