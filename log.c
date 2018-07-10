/* log.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "log.h"
#include <stdio.h>

const char* const COLOR_NORMAL  = "\033[0m";
const char* const COLOR_RED     = "\033[31m";
const char* const COLOR_GREEN   = "\033[32m";
const char* const COLOR_YELLOW  = "\033[33m";
const char* const COLOR_MAGENTA = "\033[35m";
const char* const COLOR_CYAN    = "\033[36m";

static enum LOG_LEVEL err_level = LEVEL_WARNING;

void log_setlevel(enum LOG_LEVEL level){
	err_level = level;
}

void log_msg(const char* file, int line, enum LOG_LEVEL level, const char* format, ...){
	va_list args;
	va_start(args, format);

	if (level > err_level){
		return;
	}

	switch (level){
	case LEVEL_FATAL:
		fprintf(stderr, "[%sFATAL%s](%s:%d): ", COLOR_MAGENTA, COLOR_NORMAL, file, line);
		break;
	case LEVEL_ERROR:
		fprintf(stderr, "[%sERROR%s](%s:%d): ", COLOR_RED, COLOR_NORMAL, file, line);
		break;
	case LEVEL_WARNING:
		fprintf(stderr, "[%sWARN %s](%s:%d): ", COLOR_YELLOW, COLOR_NORMAL, file, line);
		break;
	case LEVEL_DEBUG:
		fprintf(stderr, "[%sDEBUG%s](%s:%d): ", COLOR_CYAN, COLOR_NORMAL, file, line);
		break;
	case LEVEL_INFO:
		fprintf(stderr, "[%sINFO %s](%s:%d): ", COLOR_GREEN, COLOR_NORMAL, file, line);
		break;
	default:
		break;
	}

	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}
