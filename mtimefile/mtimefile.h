/** @file mtimefile/mtimefile.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __MTIMEFILE_H
#define __MTIMEFILE_H

#include "../attribute.h"
#include <stdio.h>

/**
 * @brief Adds an mtime entry to a file.
 *
 * @param file The file to add to the mtime list.
 *
 * @param fp_mtime A file pointer to an output mtime list.<br>
 * This file pointer must be opened in writing binary ("wb") mode.
 *
 * @return 0 on success, negative on failure
 */
int mtime_add_to_file(const char* file, FILE* fp_mtime) EZB_HOT;

/**
 * @brief Checks to see if a file changed since the creation of a previous mtime file.
 *
 * @param file The file to check.
 *
 * @param fp_mtime_prev A file pointer to a previously generated mtime file.<br>
 * This file must be opened in reading binary ("rb") mode.
 *
 * @return Positive if the mtime in the list does not match the current mtime or if the file was not found in the list, 0 if the current mtime matches the mtime in the list.
 */
int mtime_file_changed(const char* file, FILE* fp_mtime_prev) EZB_HOT;

/**
 * @brief Generates a list of removed files since the creation of a previous mtime file.<br>
 * Use mtime_get_next_removed() to retrieve entries from this list.
 * @see mtime_get_next_removed()
 *
 * @param mtime_file_prev A path to a previous mtime file.
 *
 * @param out_file The output path.<br>
 * If a file already exists at this path, it will be overwritten.
 *
 * @return 0 on success, negative on failure.
 */
int mtime_create_removed_list(const char* mtime_file_prev, const char* out_file);

/**
 * @brief Gets the next filename from a removed list generated with mtime_create_removed_list()
 * @see mtime_create_removed_list()
 *
 * @param fp_removed A file pointer to a removed list generated with mtime_create_removed_list()
 * This file pointer must be opened in reading binary ("rb") mode.
 *
 * @param out A pointer to a string that will be filled with the next filename in the list.
 * If the file reaches EOF or there was an error, this will be set to NULL.
 *
 * @return 0 on success, positive on EOF, negative on failure.
 */
int mtime_get_next_removed(FILE* fp_removed, char** out);

#endif
