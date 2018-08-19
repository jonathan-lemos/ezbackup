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

#define BLACK_STR   "\033[90m"
#define RED_STR     "\033[91m"
#define GREEN_STR   "\033[92m"
#define YELLOW_STR  "\033[93m"
#define BLUE_STR    "\033[94m"
#define MAGENTA_STR "\033[95m"
#define CYAN_STR    "\033[96m"
#define WHITE_STR   "\033[97m"
#define NORMAL_STR  "\033[m"

#elif defined (DISABLE_COLORS)

#define BLACK_STR   ""
#define RED_STR     ""
#define GREEN_STR   ""
#define YELLOW_STR  ""
#define BLUE_STR    ""
#define MAGENTA_STR ""
#define CYAN_STR    ""
#define WHITE_STR   ""
#define NORMAL_STR  ""

#else

#define BLACK_STR   "\033[30m"
#define RED_STR     "\033[31m"
#define GREEN_STR   "\033[32m"
#define YELLOW_STR  "\033[33m"
#define BLUE_STR    "\033[34m"
#define MAGENTA_STR "\033[35m"
#define CYAN_STR    "\033[36m"
#define WHITE_STR   "\033[37m"
#define NORMAL_STR  "\033[m"

#endif

EZB_PURE int stream_is_tty(FILE* stream){
	return !(!isatty(fileno(stream)) && errno == ENOTTY);
}

int fprintf_color(FILE* stream, enum color c, const char* format, ...){
	va_list ap;
	int ret = 0;
	int is_tty = stream_is_tty(stream);

	va_start(ap, format);

	if (is_tty){
		switch (c){
		case COLOR_BLACK:
			fprintf(stream, BLACK_STR);
			break;
		case COLOR_RED:
			fprintf(stream, RED_STR);
			break;
		case COLOR_GREEN:
			fprintf(stream, GREEN_STR);
			break;
		case COLOR_YELLOW:
			fprintf(stream, YELLOW_STR);
			break;
		case COLOR_BLUE:
			fprintf(stream, BLUE_STR);
			break;
		case COLOR_MAGENTA:
			fprintf(stream, MAGENTA_STR);
			break;
		case COLOR_CYAN:
			fprintf(stream, CYAN_STR);
			break;
		case COLOR_WHITE:
			fprintf(stream, WHITE_STR);
			break;
			/* shut up gcc */
		default:
			;
		}
	}

	ret = vfprintf(stream, format, ap);

	if (is_tty){
		fprintf(stream, NORMAL_STR);
	}

	va_end(ap);
	return ret;
}
