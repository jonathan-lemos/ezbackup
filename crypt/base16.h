/* base16.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CRYPT_BASE16_H
#define __CRYPT_BASE16_H

int to_base16(const void* bytes, unsigned len, char** out);
int from_base16(const char* hex, unsigned len, void** out, unsigned* out_len);

#endif
