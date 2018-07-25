/* readline_include.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __READLINE_INCLUDE_H
#define __READLINE_INCLUDE_H

#if defined(__linux__) || defined(__APPLE__)
#include <editline/readline.h>
#elif defined(__OpenBSD__)
#include <readline/readline.h>
#else
#error "This operating system does not support readline"
#endif

/**
 * @brief Do not call this function directly.<br>
 * Instead, use the readline() macro.<br>
 * <br>
 * This function is necessary because some implementations of readline place an extraneous ' ' after a tab completion.<br>
 * This makes readline extremely frustrating to use, as you have to backspace before typing the next component of the path.<br>
 * This function makes sure no extraneous characters are appended after a tab completion.
 * @see readline()
 *
 * @param prompt The prompt to display to the user.
 *
 * @return The user's input, or NULL on error.<br>
 * This value does not include the trailing '\\n'.<br>
 * This value must be free()'d when no longer in use.
 */
char* __mreadline(const char* prompt);

/**
 * @brief Reads a line from stdin with tab completion.<br>
 * Tab completion is very similar to what bash provides by default.<br>
 * Pressing tab will automatically complete a path.<br>
 * If more than one path can be completed, pressing tab again will show all possibilities.
 *
 * @param prompt The prompt to display to the user.
 *
 * @return The user's input, or NULL on error.<br>
 * This value does not include the trailing '\\n'.<br>
 * This value must be free()'d when no longer in use.
 */
#define readline(prompt) __mreadline(prompt)

#endif
