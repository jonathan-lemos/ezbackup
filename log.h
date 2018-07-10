/* log.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __LOG_H
#define __LOG_H

#include <stdarg.h>

/**
 * @brief The level of severity of a log message.
 */
enum LOG_LEVEL{
	LEVEL_NONE    = 0, /**< Pass to log_setlevel() to silence all log output. @see log_setlevel() */
	LEVEL_FATAL   = 1, /**< Use this for errors that the program has no chance of recovering from. */
	LEVEL_ERROR   = 2, /**< Use this for general errors. */
	LEVEL_WARNING = 3, /**< Use this for abnormalities that may or may not cause errors. */
	LEVEL_DEBUG   = 4, /**< Use this for abnormalities that are most likely fine, but could be evidence of a problem. */
	LEVEL_INFO    = 5  /**< Use this for general information about the program's state. */
};

/**
 * @brief Sets the current logging level.
 * All messages greater than the current level are silenced.
 *
 * This function is not thread-safe.
 * TODO: Make this thread-safe if I ever add multithreading to this program.
 *
 * @param level The logging level to set.
 *
 * @return void
 */
void log_setlevel(enum LOG_LEVEL level);

/**
 * @brief Logs a message to stderr.
 * Do not call this function directly. Use one of the macros provided by this library.
 *
 * @param file The file where the error takes place.
 * @param line The line where the error takes place.
 * @param level The logging level of the current entry.
 * @param format A printf format string.
 *
 * @return void
 */
void log_msg(const char* file, int line, enum LOG_LEVEL level, const char* format, ...);

/**
 * @brief Logs a LEVEL_FATAL message.
 *
 * @param msg The message.
 *
 * @return void
 */
#define log_fatal(msg) log_msg(__FILE__, __LINE__, LEVEL_FATAL, msg)

/**
 * @brief Logs a LEVEL_ERROR message.
 *
 * @param msg The message.
 *
 * @return void
 */
#define log_error(msg) log_msg(__FILE__, __LINE__, LEVEL_ERROR, msg)

/**
 * @brief Logs a LEVEL_WARNING message.
 *
 * @param msg The message.
 *
 * @return void
 */
#define log_warning(msg) log_msg(__FILE__, __LINE__, LEVEL_WARNING, msg)

/**
 * @brief Logs a LEVEL_DEBUG message.
 *
 * @param msg The message.
 *
 * @return void
 */
#define log_debug(msg) log_msg(__FILE__, __LINE__, LEVEL_DEBUG, msg)

/**
 * @brief Logs a LEVEL_INFO message.
 *
 * @param msg The message.
 *
 * @return void
 */
#define log_info(msg) log_msg(__FILE__, __LINE__, LEVEL_INFO, msg)

/**
 * @brief Logs a LEVEL_FATAL message with a single printf parameter.
 * Unfortunately, C89 does not support variadic macros, which is why this function exists.
 *
 * @param msg The message as a printf format string.
 * @param strerr The printf parameter.
 *
 * @return void
 */
#define log_fatal_ex(msg, strerr) log_msg(__FILE__, __LINE__, LEVEL_FATAL, msg, strerr)

/**
 * @brief Logs a LEVEL_ERROR message with a single printf parameter.
 * Unfortunately, C89 does not support variadic macros, which is why this function exists.
 *
 * @param msg The message as a printf format string.
 * @param strerr The printf parameter.
 *
 * @return void
 */
#define log_error_ex(msg, strerr) log_msg(__FILE__, __LINE__, LEVEL_ERROR, msg, strerr)

/**
 * @brief Logs a LEVEL_WARNING message with a single printf parameter.
 * Unfortunately, C89 does not support variadic macros, which is why this function exists.
 *
 * @param msg The message as a printf format string.
 * @param strerr The printf parameter.
 *
 * @return void
 */
#define log_warning_ex(msg, strerr) log_msg(__FILE__, __LINE__, LEVEL_WARNING, msg, strerr)

/**
 * @brief Logs a LEVEL_DEBUG message with a single printf parameter.
 * Unfortunately, C89 does not support variadic macros, which is why this function exists.
 *
 * @param msg The message as a printf format string.
 * @param strerr The printf parameter.
 *
 * @return void
 */
#define log_debug_ex(msg, strerr) log_msg(__FILE__, __LINE__, LEVEL_DEBUG, msg, strerr)

/**
 * @brief Logs a LEVEL_INFO message with a single printf parameter.
 * Unfortunately, C89 does not support variadic macros, which is why this function exists.
 *
 * @param msg The message as a printf format string.
 * @param strerr The printf parameter.
 *
 * @return void
 */
#define log_info_ex(msg, strerr) log_msg(__FILE__, __LINE__, LEVEL_INFO, msg, strerr)

/**
 * @brief Logs a LEVEL_FATAL message with a two printf parameters.
 * Unfortunately, C89 does not support variadic macros, which is why this function exists.
 *
 * @param msg The message as a printf format string.
 * @param file The first printf parameter.
 * @param strerr The second printf parameter.
 *
 * @return void
 */
#define log_fatal_ex2(msg, file, strerr) log_msg(__FILE__, __LINE__, LEVEL_FATAL, msg, file, strerr)

/**
 * @brief Logs a LEVEL_ERROR message with a two printf parameters.
 * Unfortunately, C89 does not support variadic macros, which is why this function exists.
 *
 * @param msg The message as a printf format string.
 * @param file The first printf parameter.
 * @param strerr The second printf parameter.
 *
 * @return void
 */
#define log_error_ex2(msg, file, strerr) log_msg(__FILE__, __LINE__, LEVEL_ERROR, msg, file, strerr)

/**
 * @brief Logs a LEVEL_WARNING message with a two printf parameters.
 * Unfortunately, C89 does not support variadic macros, which is why this function exists.
 *
 * @param msg The message as a printf format string.
 * @param file The first printf parameter.
 * @param strerr The second printf parameter.
 *
 * @return void
 */
#define log_warning_ex2(msg, file, strerr) log_msg(__FILE__, __LINE__, LEVEL_WARNING, msg, file, strerr)

/**
 * @brief Logs a LEVEL_DEBUG message with a two printf parameters.
 * Unfortunately, C89 does not support variadic macros, which is why this function exists.
 *
 * @param msg The message as a printf format string.
 * @param file The first printf parameter.
 * @param strerr The second printf parameter.
 *
 * @return void
 */
#define log_debug_ex2(msg, file, strerr) log_msg(__FILE__, __LINE__, LEVEL_DEBUG, msg, file, strerr)

/**
 * @brief Logs a LEVEL_INFO message with a two printf parameters.
 * Unfortunately, C89 does not support variadic macros, which is why this function exists.
 *
 * @param msg The message as a printf format string.
 * @param file The first printf parameter.
 * @param strerr The second printf parameter.
 *
 * @return void
 */
#define log_info_ex2(msg, file, strerr) log_msg(__FILE__, __LINE__, LEVEL_INFO, msg, file, strerr)

/**
 * @brief Logs an out-of-memory error.
 *
 * @return void
 */
#define log_enomem() log_msg(__FILE__, __LINE__, LEVEL_FATAL, "Failed to allocate requested memory.")

/**
 * @brief Logs that an argument was NULL when it wasn't supposed to be.
 *
 * @param arg The NULL argument.
 *
 * @return void
 */
#define log_enull(arg) log_msg(__FILE__, __LINE__, LEVEL_DEBUG, "Argument \"%s\" was NULL", #arg);

/**
 * @brief Logs that an integral argument's value was invalid.
 *
 * @param arg The invalid argument
 *
 * @return void
 */
#define log_einval(arg) log_msg(__FILE__, __LINE__, LEVEL_ERROR, "Invalid argument (%s was %ld)", #arg, (long)arg)

/**
 * @brief Logs that an unsigned argument's value was invalid.
 *
 * @param arg The invalid argument
 *
 * @return void
 */
#define log_einval_u(arg) log_msg(__FILE__, __LINE__, LEVEL_ERROR, "Invalid argument (%s was %lu)", #arg, (unsigned long)arg)

 /**
 * @brief Logs that fopen() failed.
 *
 * @param file The name of the file that failed to open.
 *
 * @return void
 */
#define log_efopen(file) log_msg(__FILE__, __LINE__, LEVEL_ERROR, "Error opening %s (%s)", file, strerror(errno))

/**
 * @brief Logs that an fwrite() failed.
 *
 * @param file The name of the file that failed to be written to.
 *
 * @return void
 */
#define log_efwrite(file) log_msg(__FILE__, __LINE__, LEVEL_ERROR, "Error writing to %s", file)

/**
 * @brief Logs that an fread() failed.
 *
 * @param file The name of the file that failed to be read from.
 *
 * @return void
 */
#define log_efread(file) log_msg(__FILE__, __LINE__, LEVEL_ERROR, "Error reading from %s", file)

/**
 * @brief Logs that an fclose() failed.
 *
 * @param file The name of the file that failed to close.
 *
 * @return void
 */
#define log_efclose(file) log_msg(__FILE__, __LINE__, LEVEL_WARNING, "Error closing %s. Data corruption possible.", file)

/**
 * @brief Logs that a FILE* is opened in the incorrect mode.
 *
 * @return void
 */
#define log_emode() log_msg(__FILE__, __LINE__, LEVEL_ERROR, "A file pointer is opened in the incorrect mode")

/**
 * @brief Logs that a stat() failed.
 *
 * @param file The name of the file that failed to stat.
 *
 * @return void
 */
#define log_estat(file) log_msg(__FILE__, __LINE__, LEVEL_DEBUG, "Failed to stat %s (%s)", file, strerror(errno))

/**
 * @brief Logs that a temp_fopen() failed.
 *
 * @return void
 */
#define log_etmpfopen() log_msg(__FILE__, __LINE__, LEVEL_ERROR, "Failed to create temporary file")

/**
 * @brief Returns ret if arg is NULL.
 *
 * @param arg The argument that may be NULL.
 *
 * @param ret The return value if arg is NULL.
 *
 * @return ret if arg is NULL.
 */
#define return_ifnull(arg, ret) if (!arg){ log_enull(arg); return ret; }

#endif
