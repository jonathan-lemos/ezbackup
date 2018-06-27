/* stringhelper.h -- helper functions for c-strings
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __STRINGHELPER_H
#define __STRINGHELPER_H

char* sh_new(void);
char* sh_dup(const char* in);
char* sh_concat(char* in, const char* extension);
const char* sh_filename(const char* in);
const char* sh_file_ext(const char* in);
const char* sh_file_name(const char* in);
int sh_starts_with(const char* haystack, const char* needle);
char* sh_getcwd(void);
int sh_cmp_nullsafe(const char* str1, const char* str2);

#endif
