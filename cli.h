/* cli.h -- cli dialog/menu
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CLI_H
#define __CLI_H

int display_dialog(const char* const* choices, int n_choices, const char* msg);
int display_menu(const char* const* options, int n_options, const char* title);

#endif
