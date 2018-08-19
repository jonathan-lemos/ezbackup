/** @file log.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "color.h"
#include "log.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

static enum LOG_LEVEL err_level = LEVEL_WARNING;

void log_setlevel(enum LOG_LEVEL level){
	err_level = level;
}

void log_msg(const char* file, int line, const char* func, enum LOG_LEVEL level, const char* format, ...){
	va_list args;

	va_start(args, format);

	if (level > err_level){
		return;
	}

	fprintf(stderr, "[");

	switch (level){
	case LEVEL_FATAL:
		fprintf_color(stderr, COLOR_MAGENTA, "FATAL");
		break;
	case LEVEL_ERROR:
		fprintf_color(stderr, COLOR_RED, "ERROR");
		break;
	case LEVEL_WARNING:
		fprintf_color(stderr, COLOR_YELLOW, "WARNING");
		break;
	case LEVEL_DEBUG:
		fprintf_color(stderr, COLOR_CYAN, "DEBUG");
		break;
	case LEVEL_INFO:
		fprintf_color(stderr, COLOR_GREEN, "INFO");
		break;
	default:
		/* shut up gcc */
		;
	}

	fprintf(stderr, "](%s:%d:%s): ", file, line, func);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}
