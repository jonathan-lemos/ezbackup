/* prototypes and #defines */
#include "error.h"
#include <stdio.h>

const char* const STR_ENOMEM  = "Failed to allocate memory.";
const char* const STR_ENULL   = "A required argument was NULL.";
const char* const STR_EFOPEN  = "Could not open %s (%s)";
const char* const STR_EFWRITE = "Failed to write to %s";
const char* const STR_EFREAD  = "Error reading from %s";
const char* const STR_EFCLOSE = "Error writing to %s";

static LOG_LEVEL err_level = LEVEL_WARNING;

void log_setlevel(LOG_LEVEL level){
	err_level = level;
}

void log_fatal(const char* format, ...){
	va_list args;
	va_start(args, format);

	if (LEVEL_FATAL > err_level){
		return;
	}

	fprintf(stderr, "Fatal error: ");
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void log_error(const char* format, ...){
	va_list args;
	va_start(args, format);

	if (LEVEL_ERROR > err_level){
		return;
	}

	fprintf(stderr, "Error: ");
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void log_warning(const char* format, ...){
	va_list args;
	va_start(args, format);

	if (LEVEL_WARNING > err_level){
		return;
	}

	fprintf(stderr, "Warning: ");
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

	fprintf(stderr, "Debug (%s:%d): ", file, line);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}
