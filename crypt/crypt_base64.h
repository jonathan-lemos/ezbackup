/* crypt_base64.h -- converts data to/from base64
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CRYPT_CRYPT_BASE64_H
#define __CRYPT_CRYPT_BASE64_H

#include <stddef.h>

unsigned char* to_base64(const unsigned char* data, size_t len, size_t* len_out);
unsigned char* from_base64(const unsigned char* data, size_t len, size_t* len_out);

#endif
