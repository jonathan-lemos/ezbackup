/** @file file/fileiterator.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __FILE_ITERATOR_H
#define __FILE_ITERATOR_H

#include "../attribute.h"

/**
 * @brief Starts iterating through files in a directory.<br>
 * If there are any subdirectories, their files/directories will also be recursively iterated through.
 *
 * @param dir The directory to start iterating in.
 *
 * @return A structure needed for the other fileiterator functions.<br>
 * This structure must be freed with fi_end() when no longer needed.
 * @see fi_next()
 * @see fi_end()
 */
struct fi_stack* fi_start(const char* dir) EZB_MALLOC_LIKE;

/**
 * @brief Returns the next filename in the fi_stack structure.
 *
 * @param fis A fi_stack* structure returned by fi_start()
 * @see fi_start()
 *
 * @return The next filename in the fi_stack* structure, or NULL if there are not any left/there was an error.
 */
char* fi_next(struct fi_stack* fis) EZB_MALLOC_LIKE;

/**
 * @brief Stops iterating files through the current directory and moves on to the next if there is one.
 *
 * @param fis A fi_stack* structure to skip the current directory on.
 * @see fi_start()
 *
 * @return 0 on success, or negative on failure.
 */
int fi_skip_current_dir(struct fi_stack* fis);

/**
 * @brief Returns the name of the current directory.
 *
 * @param fis A fi_stack* structure to get the directory name of.
 * @see fi_start()
 *
 * @return The name of the current directory, or NULL if there isn't one.
 */
const char* fi_directory_name(const struct fi_stack* fis) EZB_PURE;

/**
 * @brief Stops iterating files and frees all memory associated with the structure.
 *
 * @param fis An fi_stack* structure to free.
 * This can be NULL, in which case this function does nothing.
 *
 * @return void
 */
void fi_end(struct fi_stack* fis);

#endif
