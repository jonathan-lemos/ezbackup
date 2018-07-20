/** @file options/options_menu.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __OPTIONS_MENU_H
#define __OPTIONS_MENU_H

#include "options.h"

/**
 * @brief Creates a menu allowing the user to choose an operation.
 *
 * @return The user's selected operation, or OP_INVALID if there was an error.
 */
enum operation menu_operation(void);

/**
 * @brief Creates a menu allowing the user to edit the options in an options structure.
 *
 * @param opt The options structure to edit.
 *
 * @return 0 on success, or negative on failure.
 */
int menu_configure(struct options* opt);

#endif
