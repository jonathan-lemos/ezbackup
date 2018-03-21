/*
 * Progress bar module
 * Copyright (C) 2018 Jonathan Lemos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __PROGRESSBAR_H
#define __PROGRESSBAR_H

#include <stdint.h>
#include <time.h>

typedef struct progress{
	const char* text;
	uint64_t    count;
	uint64_t    max;
	time_t      time_prev;
}progress;

progress* start_progress(const char* text, uint64_t max);
void inc_progress(progress* p, uint64_t count);
void set_progress(progress* p, uint64_t count);
void finish_progress(progress* p);

#endif
