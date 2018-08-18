/** @file color.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "color.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#if defined(ENABLE_BRIGHT_COLORS)

#define BLACK_STR "\033[90m"
#define RED_STR "\033[91m"
#define GREEN_STR "\033[92m"
#define YELLOW_STR "\033[93m"
#define BLUE_STR "\033[94m"
#define MAGENTA_STR "\033[95m"
#define CYAN_STR "\033[96m"
#define WHITE_STR "\033[97m"
#define NORMAL_STR "\033[m"

#elif defined (DISABLE_COLORS)

#define RED_STR ""
#define YELLOW_STR ""
#define GREEN_STR ""
#define BLUE_STR ""
#define NORMAL_STR ""

#else

#define RED_STR "\033[31m"
#define YELLOW_STR "\033[33m"
#define GREEN_STR "\033[32m"
#define BLUE_STR "\033[36m"
#define NORMAL_STR "\033[m"

#endif

int stream_is_tty(FILE* stream){
	return isatty(fileno(stream)) && errno == ENOTTY;
}

int printf_color(FILE* stream, enum color c, const char* format, ...){
	va_list ap;

	va_start(ap, format);

	if (stream_is_tty(stream)){
		switch (c){
			case COLOR_BLACK:

		}
	}

	vfprintf(stderr, format, ap);

	va_end(ap);
}
