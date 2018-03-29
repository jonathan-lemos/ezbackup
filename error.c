/* prototypes and #defines */
#include "error.h"
#include <stdio.h>

const char* const STR_ENOMEM  = "Failed to allocate memory.";
const char* const STR_ENULL   = "A required argument was NULL.";
const char* const STR_EFOPEN  = "Could not open %s (%s)";
const char* const STR_EFWRITE = "Error writing to %s";
const char* const STR_EFREAD  = "Error reading from %s";
const char* const STR_EFCLOSE = "Error closing %s";
const char* const STR_EMODE = "The file pointer is opened in the incorrect mode";

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
