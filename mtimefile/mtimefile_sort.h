/** @file mtimefile_sort.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __MTIMEFILE_SORT_H
#define __MTIMEFILE_SORT_H

#include "entry.h"
#include <stdio.h>

#ifndef MAX_RUN_LEN
#define MAX_RUN_LEN (1 << 10)
#endif

/**
 * @brief Sorts an mtimefile in strcmp() order based on filename.
 *
 * @param file An existing mtime file.
 *
 * @return 0 on success, negative on failure.<br>
 * On failure, the existing file is not changed.
 */
int mtime_sort(const char* file);

/**
 * @brief Searches a sorted mtime file for a filename, returning its mtime if it exists.<br>
 * The mtime file must be sorted or this function will likely not return the correct value.
 *
 * @param fp_mtime A file pointer corresponding to a sorted mtime file.<br>
 * This file must be opened in reading binary ("rb") mode.<br>
 * The position of the internal file pointer does not matter.
 *
 * @param key The filename to search for.
 *
 * @param mtime_out A pointer to a time_t that will contain the mtime of the corresponding filename.<br>
 * If the corresponding filename could not be found, this value is not updated.
 *
 * @return 0 on success, positive if the file could not be found, negative on failure.
 */
int mtime_search(FILE* fp_mtime, const char* key, time_t* mtime_out);

/**
 * @brief Writes an entry to an mtime file.
 *
 * @param file The filename to write.
 *
 * @param mtime The mtime corresponding to the filename.
 *
 * @param fp_mtime The file pointer to write to.<br>
 * This file must be opened in writing binary ("wb") mode.
 *
 * @return 0 on success, negative on failure.
 */
int mtime_write(const char* file, time_t mtime, FILE* fp_mtime);

/**
 * @brief Reads the next entry from an mtime file.
 *
 * @param file The mtime file to read from.<br>
 * This file must be opened in reading binary ("rb") mode.
 *
 * @param file_out A pointer to a string that will be filled with a filename by this function.<br>
 * This value can be NULL, in which case it is not used.<br>
 * Otherwise, if this function fails or EOF is reached, this will be set to NULL.
 *
 * @param mtime_out A pointer to a time_t that will be filled with the corresponding time_t.<br>
 * This value can be NULL, in which case it is not used.<br>
 * Otherwise, if this function fails or EOF is reached, this will be set to 0.
 *
 * @return 0 on success, positive on EOF, negative on failure.
 */
int mtime_read(FILE* fp_mtime, char** file_out, time_t* mtime_out);

#endif
