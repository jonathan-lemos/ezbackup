/** @file strings/stringhelper.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __STRINGHELPER_H
#define __STRINGHELPER_H

#include <stdarg.h>
#include <stddef.h>

/**
 * @brief Creates an empty dynamic string.
 *
 * @return An empty dynamic string, or NULL on failure.<br>
 * This string must be free()'d when no longer in use.
 */
char* sh_new(void);

/**
 * @brief Duplicates a string or converts a static string to a dynamic one.<br>
 * This function is essentially equivalent to strdup() with more error checking.
 *
 * @param in The string to duplicate.<br>
 * This may be NULL, in which case NULL is returned.
 *
 * @return A dynamic copy of that string, or NULL on failure.<br>
 * This string must be free()'d when no longer in use.
 */
char* sh_dup(const char* in);

/**
 * @brief Concatenates an extension to a dynamic string.
 *
 * @param in A dynamic string.<br>
 * This pointer is free()'d once this function completes, regardless of if it succeeds or not.<br>
 * To use this function with const char*, pass sh_dup(const char*) to this parameter.
 * @see sh_dup()
 *
 * @param extension The string to concatenate to the input string.
 *
 * @return The concatenated string, or NULL on failure.<br>
 * This string must be free()'d when no longer in use.
 */
char* sh_concat(char* in, const char* extension);

/**
 * @brief Concatenates a path to a dynamic string.<br>
 * Examples:<br>
 * `sh_concat_path("/dir1", "/file.txt")  -> "/dir1/file.txt"`<br>
 * `sh_concat_path("/dir1", "file.txt")   -> "/dir1/file.txt"`<br>
 * `sh_concat_path("/dir1/", "/file.txt") -> "/dir1/file.txt"`<br>
 * `sh_concat_path("dir1", "file.txt")    -> "dir1/file.txt"`
 *
 * @param in A dynamic string.<br>
 * This pointer is free()'d once this function completes, regardless of if it succeeds or not.<br>
 * To use this function with const char*, pass sh_dup(const char*) to this parameter.
 * @see sh_dup()
 *
 * @return The concatenated string, or NULL on failure.<br>
 * This string must be free()'d when no longer in use.
 */
char* sh_concat_path(char* in, const char* extension);

/**
 * @brief Returns the filename for a path.<br>
 * Examples:<br>
 * `sh_filename("/home/equifax/passwords.txt"); -> "passwords.txt"`<br>
 * `sh_filename("/home/equifax/passwords/");    -> "passwords/"`<br>
 * `sh_filename("equifax_passwords.txt");       -> "equifax_passwords.txt"`
 *
 * @param in A string containing a path.<br>
 * This path does not necessarily have to exist.
 *
 * @return The path's corresponding filename.<br>
 * Note that this value is a pointer within the input string.<br>
 * Therefore if the input string is changed, this pointer is no longer valid.
 */
const char* sh_filename(const char* in);

/**
 * @brief Returns the file extension for a path.<br>
 * Examples:<br>
 * `sh_file_ext("/home/equifax/passwords.txt") -> ".txt"`<br>
 * `sh_file_ext("/home/equifax/passwords/")    -> "passwords/"`<br>
 * `sh_file_ext("/home/equifax/passwords")     -> "passwords"`
 *
 * @param in A string containing a path.<br>
 * This path does not necessarily have to exist.
 *
 * @return The path's corresponding extension.<br>
 * Note that this value is a pointer within the input string.<br>
 * Therefore if the input string is changed, this pointer is no longer valid.
 */
const char* sh_file_ext(const char* in);

/**
 * @brief Returns the parent directory for a path.<br>
 * Examples:<br>
 * `sh_parent_dir("/home/equifax/passwords.txt") -> "/home/equifax"`<br>
 * `sh_parent_dir("/home/equifax/passwords/")    -> "/home/equifax"`<br>
 * `sh_parent_dir("/home")                       -> "/"`<br>
 * `sh_parent_dir("/")                           -> NULL`
 *
 * @param in A string containing a path.<br>
 * This path does not necessarily have to exist.
 *
 * @return The path's parent directory, or NULL if it does not exist or there was an error.<br>
 * This string must be free()'d when no longer in use.
 */
char* sh_parent_dir(const char* in);

/**
 * @brief Returns true if the haystack starts with the needle, or false if not.<br>
 * Examples:<br>
 * `sh_starts_with("hunter2", "hunt")     -> true`<br>
 * `sh_starts_with("hunter2", "h")        -> true`<br>
 * `sh_starts_with("hunter2", "hunter2")  -> true`<br>
 * `sh_starts_with("hunter2", "hunter23") -> false`<br>
 * `sh_starts_with("hunter2", "Hunter2")  -> false`
 *
 * @param haystack The string to search within.
 *
 * @param needle The substring to search for.
 *
 * @return 1 if haystack starts with needle, 0 if not.
 */
int sh_starts_with(const char* haystack, const char* needle);

/**
 * @brief Gets the current directory as a dynamic string.<br>
 *
 * @return The current directory, or NULL on error.<br>
 * This string must be free()'d when no longer in use.
 */
char* sh_getcwd(void);

/**
 * @brief Compares two strings that may or may not be NULL.<br>
 *
 * @param str1 The first string.<br>
 * This can be NULL.
 *
 * @param str2 The second string.<br>
 * This can be NULL.
 *
 * @return For two non-null strings, the return value is the same as strcmp().<br>
 * Otherwise:<br>
 * `sh_cmp_nullsafe(NULL, "non-null")  > 0`<br>
 * `sh_cmp_nullsafe("non-null", NULL)  < 0`<br>
 * `sh_cmp_nullsafe(NULL, NULL)       == 0`
 */
int sh_cmp_nullsafe(const char* str1, const char* str2);

/**
 * @brief Case-insensitively compares two strings that may or may not be NULL.
 *
 * @param str1 The first string.<br>
 * This can be NULL.
 *
 * @param str2 The second string.<br>
 * This can be NULL
 *
 * @return Same as sh_cmp_nullsafe() while treating capital letters as equal to lowercase ones.<br>
 * @see sh_cmp_nullsafe()
 */
int sh_ncasecmp(const char* str1, const char* str2);

/**
 * @brief Creates a string out of a printf statement.<br>
 * This function is like sprintf(), but it allocates the destination string instead of requiring a static buffer.
 *
 * @param format A printf format string.
 *
 * @param ... The printf format string's arguments, if any.
 *
 * @return A string corresponding to the format string filled in with its arguments, or NULL on error.<br>
 * This string must be free()'d when no longer in use.
 */
char* sh_sprintf(const char* format, ...);

#endif
