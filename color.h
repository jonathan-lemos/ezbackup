/** @file color.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __COLOR_H
#define __COLOR_H

#include "attribute.h"
#include <stdarg.h>
#include <stdio.h>

/**
 * @brief Colors for use with fprintf_color()
 */
enum color{
	COLOR_DEFAULT, /**< The default color. */
	COLOR_BLACK,   /**< Black */
	COLOR_RED,     /**< Red */
	COLOR_GREEN,   /**< Green */
	COLOR_YELLOW,  /**< Yellow */
	COLOR_BLUE,    /**< Blue */
	COLOR_MAGENTA, /**< Magenta */
	COLOR_CYAN,    /**< Cyan */
	COLOR_WHITE    /**< White */
};

/**
 * @brief Prints to a stream in color.<br>
 * If the stream does not point to a terminal, the color is omitted.
 *
 * @param stream The stream to print to.<br>
 * This should be stdout or stderr to print to the terminal.<br>
 * Otherwise, this stream needs to be opened in writing ("w") mode.
 *
 * @param c The color to print in.
 * @see enum color
 *
 * @param format A printf format string.
 *
 * @param ... The printf format args, if any.
 *
 * @return The number of characters successfully written.
 */
int fprintf_color(FILE* stream, enum color c, const char* format, ...) EZB_PRINTF_LIKE(3);

#endif
