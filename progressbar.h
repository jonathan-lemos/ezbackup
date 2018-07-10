/* progressbar.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __PROGRESSBAR_H
#define __PROGRESSBAR_H

#include <stdint.h>
#include <time.h>

struct progress{
	const char* text;
	uint64_t    count;
	uint64_t    max;
	time_t      time_prev;
};

struct progress* start_progress(const char* text, uint64_t max);
void inc_progress(struct progress* p, uint64_t count);
void set_progress(struct progress* p, uint64_t count);
void finish_progress(struct progress* p);
void finish_progress_fail(struct progress* p);

#endif
