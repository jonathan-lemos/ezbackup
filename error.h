#ifndef __ERROR_H
#define __ERROR_H

#include <stdarg.h>

enum LOG_LEVEL{
	LEVEL_NONE    = 0,
	LEVEL_FATAL   = 1,
	LEVEL_ERROR   = 2,
	LEVEL_WARNING = 3,
	LEVEL_DEBUG   = 4,
	LEVEL_INFO    = 5
};

void log_setlevel(enum LOG_LEVEL level);
void log_msg(const char* file, int line, enum LOG_LEVEL level, const char* format, ...);

#define log_fatal(msg) log_msg(__FILE__, __LINE__, LEVEL_FATAL, msg)
#define log_error(msg) log_msg(__FILE__, __LINE__, LEVEL_ERROR, msg)
#define log_warning(msg) log_msg(__FILE__, __LINE__, LEVEL_WARNING, msg)
#define log_debug(msg) log_msg(__FILE__, __LINE__, LEVEL_DEBUG, msg)
#define log_info(msg) log_msg(__FILE__, __LINE__, LEVEL_INFO, msg)

#define log_fatal_ex(msg, strerr) log_msg(__FILE__, __LINE__, LEVEL_FATAL, msg, strerr)
#define log_error_ex(msg, strerr) log_msg(__FILE__, __LINE__, LEVEL_ERROR, msg, strerr)
#define log_warning_ex(msg, strerr) log_msg(__FILE__, __LINE__, LEVEL_WARNING, msg, strerr)
#define log_debug_ex(msg, strerr) log_msg(__FILE__, __LINE__, LEVEL_DEBUG, msg, strerr)
#define log_info_ex(msg, strerr) log_msg(__FILE__, __LINE__, LEVEL_INFO, msg, strerr)

#define log_enomem() log_msg(__FILE__, __LINE__, LEVEL_FATAL, "Failed to allocate requested memory. The system may be out of memory.")
#define log_enull() log_msg(__FILE__, __LINE__, LEVEL_ERROR, "A required argument was NULL");
#define log_efopen(file) log_msg(__FILE__, __LINE__, LEVEL_ERROR, "Error opening %s (%s)", file, strerror(errno))
#define log_efwrite(file) log_msg(__FILE__, __LINE__, LEVEL_ERROR, "Error writing to %s", file)
#define log_efread(file) log_msg(__FILE__, __LINE__, LEVEL_ERROR, "Error reading from %s", file)
#define log_efclose(file) log_msg(__FILE__, __LINE__, LEVEL_WARNING, "Error closing %s. Data corruption possible.", file)
#define log_emode() log_msg(__FILE__, __LINE__, LEVEL_ERROR, "A file pointer is opened in the incorrect mode")
#define log_estat(file) log_msg(__FILE__, __LINE__, LEVEL_ERROR, "Failed to stat %s (%s)", file, strerror(errno))

#endif
