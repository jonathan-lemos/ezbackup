/** @file progressbar.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __PROGRESSBAR_H
#define __PROGRESSBAR_H

#include "attribute.h"
#include <stdint.h>
#include <time.h>

/**
 * @brief A progress bar structure.
 */
struct progress{
	const char* text;      /**< @brief The message to display above the progress bar. */
	uint64_t    count;     /**< @brief The current progress level. */
	uint64_t    max;       /**< @brief The maximum progress level. */
	time_t      time_prev; /**< @brief The current time. This is used to make sure the progressbar only updates once per second. */
};

/**
 * @brief Starts a progress bar on stdout.<br>
 *
 * This progress bar is not thread-safe if other threads write to stdout.
 *
 * @param text The message of the progress bar.
 *
 * @param max The maximum value of the progress bar.
 *
 * @return A progress bar structure.<br>
 * This progress bar must be freed with finish_progress() or finish_progress_fail() when it is no longer needed.
 * @see finish_progress()
 * @see finish_progress_fail()
 */
struct progress* start_progress(const char* text, uint64_t max) EZB_MALLOC_LIKE;

/**
 * @brief Increments the progress in a progress bar.
 *
 * @param p A progress bar structure.
 *
 * @param count The amount to increment the progress by.
 *
 * @return void
 */
void inc_progress(struct progress* p, uint64_t count);

/**
 * @brief Sets the progress in a progress bar to a specified value.
 *
 * @param p A progress bar structure.
 *
 * @param count The value to set the progress bar to.
 *
 * @return void
 */
void set_progress(struct progress* p, uint64_t count);

/**
 * @brief Sets the progress to 100%, ends a progress bar, and frees all memory associated with it.
 *
 * @param p The progress bar to end.
 *
 * @return void
 */
void finish_progress(struct progress* p);

/**
 * @brief Leaves the progress where it is, ends a progress bar, and frees all memory associated with it.
 *
 * @param p The progress bar to end.
 *
 * @return void
 */
void finish_progress_fail(struct progress* p);

#endif
