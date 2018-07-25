/* readline_include.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "readline_include.h"

#undef readline

char* __mreadline(const char* prompt){
	rl_completion_append_character = '\0';
	return readline(prompt);
}
