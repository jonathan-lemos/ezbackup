/** @file log.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __LOG_H
#define __LOG_H

#include <stdarg.h>
#include <string.h>
#include <errno.h>

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
 * @brief Sets the current logging level.<br>
 * All messages greater than the current level are silenced.<br>
 * This function is not thread-safe.
 * @see enum LOG_LEVEL
 *
 * @param level The logging level to set.
 *
 * @return void
 */
void log_setlevel(enum LOG_LEVEL level);

/**
 * @brief Logs a message to stderr.<br>
 * Do not call this function directly. Use one of the macros provided by this library.
 *
 * @param file The file where the error takes place.
 *
 * @param line The line where the error takes place.
 *
 * @param level The logging level of the current entry.
 *
 * @param format A printf format string.
 *
 * @return void
 */
void log_msg(const char* file, int line, const char* func, enum LOG_LEVEL level, const char* format, ...);

/**
 * @brief Logs a LEVEL_FATAL message.
 *
 * @param msg The message.
 *
 * @return void
 */
#define log_fatal(...) log_msg(__FILE__, __LINE__, __func__, LEVEL_FATAL, __VA_ARGS__)

/**
 * @brief Logs a LEVEL_ERROR message.
 *
 * @param msg The message.
 *
 * @return void
 */
#define log_error(...) log_msg(__FILE__, __LINE__, __func__, LEVEL_ERROR, __VA_ARGS__)

/**
 * @brief Logs a LEVEL_WARNING message.
 *
 * @param msg The message.
 *
 * @return void
 */
#define log_warning(...) log_msg(__FILE__, __LINE__, __func__, LEVEL_WARNING, __VA_ARGS__)

/**
 * @brief Logs a LEVEL_DEBUG message.
 *
 * @param msg The message.
 *
 * @return void
 */
#define log_debug(...) log_msg(__FILE__, __LINE__, LEVEL_DEBUG, __VA_ARGS__)

/**
 * @brief Logs a LEVEL_INFO message.
 *
 * @param msg The message.
 *
 * @return void
 */
#define log_info(...) log_msg(__FILE__, __LINE__, __func__, LEVEL_INFO, __VA_ARGS__)

/**
 * @brief Logs an out-of-memory error.
 *
 * @return void
 */
#define log_enomem() log_msg(__FILE__, __LINE__, __func__,  LEVEL_FATAL, "The system could not allocate the requested memory.")

/**
 * @brief Logs that an argument was NULL when it wasn't supposed to be.
 *
 * @param arg The NULL argument.
 *
 * @return void
 */
#define log_enull(arg) log_msg(__FILE__, __LINE__, __func__, LEVEL_DEBUG, "Argument \"%s\" was NULL", #arg);

/**
 * @brief Logs that an integral argument's value was invalid.
 *
 * @param arg The invalid argument
 *
 * @return void
 */
#define log_einval(arg) log_msg(__FILE__, __LINE__, __func__, LEVEL_ERROR, "Invalid argument (%s was %lld)", #arg, (long long)arg)

/**
 * @brief Logs that an unsigned argument's value was invalid.
 *
 * @param arg The invalid argument
 *
 * @return void
 */
#define log_einval_u(arg) log_msg(__FILE__, __LINE__, __func__, LEVEL_ERROR, "Invalid argument (%s was %llu)", #arg, (unsigned long long)arg)

 /**
 * @brief Logs that fopen() failed.
 *
 * @param file The name of the file that failed to open.
 *
 * @return void
 */
#define log_efopen(file) log_msg(__FILE__, __LINE__, __func__, LEVEL_WARNING, "Error opening %s (%s)", file, strerror(errno))

/**
 * @brief Logs that an fwrite() failed.
 *
 * @param file The name of the file that failed to be written to.
 *
 * @return void
 */
#define log_efwrite(file) log_msg(__FILE__, __LINE__, __func__, LEVEL_ERROR, "Error writing to %s", file)

/**
 * @brief Logs that an fread() failed.
 *
 * @param file The name of the file that failed to be read from.
 *
 * @return void
 */
#define log_efread(file) log_msg(__FILE__, __LINE__, __func__, LEVEL_ERROR, "Error reading from %s", file)

/**
 * @brief Logs that an fclose() failed.
 *
 * @param file The name of the file that failed to close.
 *
 * @return void
 */
#define log_efclose(file) log_msg(__FILE__, __LINE__, __func__, LEVEL_WARNING, "Error closing %s. Data corruption possible.", file)

/**
 * @brief Logs that a FILE* is opened in the incorrect mode.
 *
 * @return void
 */
#define log_emode() log_msg(__FILE__, __LINE__, __func__, LEVEL_ERROR, "A file pointer is opened in the incorrect mode")

/**
 * @brief Logs that a stat() failed.
 *
 * @param file The name of the file that failed to stat.
 *
 * @return void
 */
#define log_estat(file) log_msg(__FILE__, __LINE__, __func__, LEVEL_DEBUG, "Failed to stat %s (%s)", file, strerror(errno))

/**
 * @brief Logs that a temp_fopen() failed.
 *
 * @return void
 */
#define log_etmpfopen() log_msg(__FILE__, __LINE__, __func__, LEVEL_ERROR, "Failed to create temporary file")

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
