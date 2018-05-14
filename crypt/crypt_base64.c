/* crypt_base64.c -- converts data to/from base64
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "crypt_base64.h"
#include "../error.h"
#include "../stringhelper.h"
#include <stddef.h>
#include <string.h>
#include <stdint.h>

unsigned char* to_base64(const unsigned char* data, size_t len, size_t* len_out){
	char base64_map[64] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '+', '/'
	};
	char* ret;
	char buf_out[5];
	size_t i;

	ret = sh_new();
	if (!ret){
		log_enomem();
		return NULL;
	}

	buf_out[4] = '\0';
	for (i = 0; i < len - 2; i += 3){
		uint_fast32_t buf;
		buf  = data[i + 2];
		buf += data[i + 1] << 8;
		buf += data[i    ] << 16;
		buf_out[0] = base64_map[(buf & 0x00FC0000) >> 18];
		buf_out[1] = base64_map[(buf & 0x0003F000) >> 12];
		buf_out[2] = base64_map[(buf & 0x00000FC0) >> 6];
		buf_out[3] = base64_map[(buf & 0x0000003F)];
		ret = sh_concat(ret, buf_out);
	}

	if (len - i == 1){
		buf_out[0] = 0;
		buf_out[1] = 0;
		buf_out[2] = base64_map[(data[i] & 0x00000FC0) >> 6];
		buf_out[3] = base64_map[(data[i] & 0x0000003F)];
		ret = sh_concat(ret, buf_out + 2);
		ret = sh_concat(ret, "==");
	}
	else if (len - i == 2){
		uint_fast32_t buf;
		buf  = data[i + 1];
		buf += data[i    ] << 8;

		buf_out[0] = 0;
		buf_out[1] = base64_map[(buf & 0x0003F000) >> 12];
		buf_out[2] = base64_map[(buf & 0x00000FC0) >> 6];
		buf_out[3] = base64_map[(buf & 0x0000003F)];
		ret = sh_concat(ret, buf_out + 1);
		ret = sh_concat(ret, "=");
	}

	if (!ret){
		log_enomem();
		return NULL;
	}

	*len_out = strlen(ret);
	return (unsigned char*)ret;
}

unsigned char convert_base64_char(unsigned char uc){
	if (uc >= 'A' && uc <= 'Z'){
		return uc - 'A';
	}
	else if (uc >= 'a' && uc <= 'z'){
		return (uc - 'a') + ('Z' - 'A') + 1;
	}
	else if (uc >= '0' && uc <= '9'){
		return (uc - '0') + ('z' - 'a') + ('Z' - 'A') + 1;
	}
	else if (uc == '+'){
		return 62;
	}
	else{
		return 63;
	}
}

unsigned char* from_base64(const unsigned char* data, size_t len, size_t* len_out){
	char* ret;
	char buf_out[4];
	size_t i;

	ret = sh_new();
	if (!ret){
		log_enomem();
		return NULL;
	}

	buf_out[3] = '\0';
	for (i = 0; i < len - 4; i += 4){
		uint_fast32_t buf;
		buf  = convert_base64_char(data[i + 3]);
		buf += convert_base64_char(data[i + 2]) << 6;
		buf += convert_base64_char(data[i + 1]) << 12;
		buf += convert_base64_char(data[i    ]) << 18;
		buf_out[0] = (buf & 0x00FF0000) >> 16;
		buf_out[1] = (buf & 0x0000FF00) >> 8;
		buf_out[2] = (buf & 0x000000FF);
		ret = sh_concat(ret, buf_out);
	}

	if (data[len - 2] == '='){
		uint_fast16_t buf;
		buf  = convert_base64_char(data[len - 3]);
		buf += convert_base64_char(data[len - 4]) << 6;
		buf_out[0] = (buf & 0xFF00) >> 8;
		buf_out[1] = (buf & 0x00FF);
		buf_out[2] = '\0';
		ret = sh_concat(ret, buf_out);
	}

	*len_out = strlen(ret);
	return (unsigned char*)ret;
}
