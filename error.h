#ifndef __EVPERROR_H
#define __EVPERROR_H

#include <stdarg.h>

const char* STR_ENOMEM  = "Failed to allocate memory.";
const char* STR_NULLARG = "A required argument was NULL.";
const char* STR_BADFILE = "Could not open %s (%s)";

typedef enum LOG_LEVEL{
	LEVEL_NONE    = 0,
	LEVEL_FATAL   = 1,
	LEVEL_ERROR   = 2,
	LEVEL_WARNING = 3,
	LEVEL_DEBUG   = 4
}LOG_LEVEL;

void log_setlevel(LOG_LEVEL level);
void log_fatal(const char* format, ...);
void log_error(const char* format, ...);
void log_warning(const char* format, ...);
#define puts_debug(str) log_debug(__FILE__, __LINE__, str)
void log_debug(const char* file, int line, const char* format, ...);

#endif
