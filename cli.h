/* cli.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CLI_H
#define __CLI_H

/**
 * @brief Displays a dialog and returns the index of the option chosen.
 *
 * @param choices An array of strings containing the user's choices (e.g. {"Abort", "Retry", "Fail"}).
 *
 * @param n_choices The number of strings in the choices array.
 *
 * @param msg The message to display in the dialog.
 *
 * @return The index of the chosen option, or negative on failure.
 */
int display_dialog(const char* const* choices, int n_choices, const char* msg);

/**
 * @brief Displays a menu and returns the index of the option chosen.
 *
 * @param choices An array of strings containing the user's choices.
 *
 * @param n_choices The number of strings in the choices array.
 *
 * @return The index of the chosen option, or negative on failure.
 */
int display_menu(const char* const* options, int n_options, const char* title);

#endif
