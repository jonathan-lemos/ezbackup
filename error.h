#ifndef __ERROR_H
#define __ERROR_H

#include <stdarg.h>

extern const char* const STR_ENOMEM;
extern const char* const STR_ENULL;
extern const char* const STR_EFOPEN;
extern const char* const STR_EFWRITE;
extern const char* const STR_EFREAD;
extern const char* const STR_EFCLOSE;

typedef enum LOG_LEVEL{
	LEVEL_NONE    = 0,
	LEVEL_FATAL   = 1,
	LEVEL_ERROR   = 2,
	LEVEL_WARNING = 3,
	LEVEL_DEBUG   = 4,
	LEVEL_INFO    = 5
}LOG_LEVEL;

#define __FL__ __FILE__, __LINE__

void log_setlevel(LOG_LEVEL level);
void log_fatal(const char* file, int line, const char* format, ...);
void log_error(const char* file, int line, const char* format, ...);
void log_warning(const char* file, int line, const char* format, ...);
void log_debug(const char* file, int line, const char* format, ...);
void log_info(const char* file, int line, const char* format, ...);

#endif
