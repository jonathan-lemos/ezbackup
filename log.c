/** @file log.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "log.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

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

void log_msg(const char* file, int line, const char* func, enum LOG_LEVEL level, const char* format, ...){
	va_list args;
	int output_is_tty = !isatty(STDERR_FILENO) && errno == ENOTTY;

	va_start(args, format);

	if (level > err_level){
		return;
	}

	if (output_is_tty){
		switch (level){
		case LEVEL_FATAL:
			fprintf(stderr, "[%sFATAL%s]", COLOR_MAGENTA, COLOR_NORMAL);
			break;
		case LEVEL_ERROR:
			fprintf(stderr, "[%sFATAL%s]", COLOR_RED, COLOR_NORMAL);
			break;
		case LEVEL_FATAL:
			fprintf(stderr, "[%sFATAL%s]", COLOR_MAGENTA, COLOR_NORMAL);
			break;
		case LEVEL_FATAL:
			fprintf(stderr, "[%sFATAL%s]", COLOR_MAGENTA, COLOR_NORMAL);
			break;
		case LEVEL_FATAL:
			fprintf(stderr, "[%sFATAL%s]", COLOR_MAGENTA, COLOR_NORMAL);
			break;
		}
	}

	switch (level){
	case LEVEL_FATAL:
		fprintf(stderr, "[%sFATAL%s](%s:%d:%s): ", COLOR_MAGENTA, COLOR_NORMAL, file, line, func);
		break;
	case LEVEL_ERROR:
		fprintf(stderr, "[%sERROR%s](%s:%d:%s): ", COLOR_RED, COLOR_NORMAL, file, line, func);
		break;
	case LEVEL_WARNING:
		fprintf(stderr, "[%sWARN %s](%s:%d:%s): ", COLOR_YELLOW, COLOR_NORMAL, file, line, func);
		break;
	case LEVEL_DEBUG:
		fprintf(stderr, "[%sDEBUG%s](%s:%d:%s): ", COLOR_CYAN, COLOR_NORMAL, file, line, func);
		break;
	case LEVEL_INFO:
		fprintf(stderr, "[%sINFO %s](%s:%d:%s): ", COLOR_GREEN, COLOR_NORMAL, file, line, func);
		break;
	default:
		break;
	}

	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}
