/*
 * Error logging module
 * Copyright (C) 2018 Jonathan Lemos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* prototypes and #defines */
#include "error.h"
#include <stdio.h>

const char* const STR_ENOMEM  = "Failed to allocate memory.";
const char* const STR_ENULL   = "A required argument was NULL.";
const char* const STR_EFOPEN  = "Could not open %s (%s)";
const char* const STR_EFWRITE = "Error writing to %s";
const char* const STR_EFREAD  = "Error reading from %s";
const char* const STR_EFCLOSE = "Error closing %s";

const char* const COLOR_NORMAL  = "\033[0m";
const char* const COLOR_RED     = "\033[31m";
const char* const COLOR_GREEN   = "\033[32m";
const char* const COLOR_YELLOW  = "\033[33m";
const char* const COLOR_MAGENTA = "\033[35m";
const char* const COLOR_CYAN    = "\033[36m";

static LOG_LEVEL err_level = LEVEL_WARNING;

void log_setlevel(LOG_LEVEL level){
	err_level = level;
}

void log_fatal(const char* file, int line, const char* format, ...){
	va_list args;
	va_start(args, format);

	if (LEVEL_FATAL > err_level){
		return;
	}

	fprintf(stderr, "[%sFATAL%s](%s:%d): ", COLOR_MAGENTA, COLOR_NORMAL, file, line);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void log_error(const char* file, int line, const char* format, ...){
	va_list args;
	va_start(args, format);

	if (LEVEL_ERROR > err_level){
		return;
	}

	fprintf(stderr, "[%sERROR%s](%s:%d): ", COLOR_RED, COLOR_NORMAL, file, line);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void log_warning(const char* file, int line, const char* format, ...){
	va_list args;
	va_start(args, format);

	if (LEVEL_WARNING > err_level){
		return;
	}

	fprintf(stderr, "[%sWARN %s](%s:%d): ", COLOR_YELLOW, COLOR_NORMAL, file, line);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void log_debug(const char* file, int line, const char* format, ...){
	va_list args;
	va_start(args, format);

	if (LEVEL_DEBUG > err_level){
		return;
	}

	fprintf(stderr, "[%sDEBUG%s](%s:%d): ", COLOR_CYAN, COLOR_NORMAL, file, line);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void log_info(const char* file, int line, const char* format, ...){
	va_list args;
	va_start(args, format);

	if (LEVEL_INFO > err_level){
		return;
	}

	fprintf(stderr, "[%sINFO %s](%s:%d): ", COLOR_GREEN, COLOR_NORMAL, file, line);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}
