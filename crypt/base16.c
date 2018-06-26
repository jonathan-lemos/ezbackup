/* base16.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "../log.h"
#include <stdlib.h>
#include <stdio.h>

int to_base16(const void* bytes, unsigned len, char** out){
	const unsigned char* ucbytes = bytes;
	unsigned i;
	unsigned outptr;
	unsigned outlen;
	const char hexmap[] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F'};

	return_ifnull(bytes, -1);
	return_ifnull(out, -1);

	/* 2 hex chars = 1 byte */
	/* +1 for null terminator */
	outlen = len * 2 + 1;
	*out = malloc(outlen);
	if (!out){
		log_enomem();
		return -1;
	}

	for (i = 0, outptr = 0; i < len; ++i, outptr += 2){
		/* NOTE TO SELF:
		 * (*out)[x] != *out[x] */

		/* converts top 4 bits to a hex digit */
		(*out)[outptr] = hexmap[(ucbytes[i] & 0xF0) >> 4];
		/* converts bottom 4 bits */
		(*out)[outptr + 1] = hexmap[(ucbytes[i] & 0x0F)];
	}
	/* since '\0' is not a valid hex char, we can null-terminate */
	(*out)[outlen - 1] = '\0';

	return 0;
}

int from_base16(const char* hex, unsigned len, void** out, unsigned* len_out){
	unsigned char** ucout = (unsigned char**)out;
	unsigned ptr;
	unsigned hexptr;
	unsigned c;
	/* 2 hex digits = 1 byte */
	out = malloc(len / 2);
	if (!out){
		log_enomem();
		return -1;
	}

	for (ptr = 0, hexptr = 0; hexptr < len; ptr++, hexptr += 2){
		sscanf(&(hex[hexptr]), "%2x", &c);
		(*ucout)[ptr] = c;
	}

	if (len_out){
		*len_out = len / 2;
	}

	return 0;
}
