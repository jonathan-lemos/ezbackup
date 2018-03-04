#ifndef __PROGRESSBAR_H
#define __PROGRESSBAR_H

typedef struct progress{
	const char* text;
	int count;
	int max;
}progress;

progress* start_progress(const char* text, int max);
void inc_progress(progress* p, int count);
void set_progress(progress* p, int count);
void finish_progress(progress* p);

#endif
