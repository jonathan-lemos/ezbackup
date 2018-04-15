/* progressbar.c -- command-line progress bar
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* prototypes */
#include "progressbar.h"
/* error handling */
#include "error.h"
/* terminal width */
#include <sys/ioctl.h>
/* STDOUT_FILENO */
#include <unistd.h>
/* printf */
#include <stdio.h>
/* malloc */
#include <stdlib.h>
/* strlen */
#include <string.h>

static int get_width(void){
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	return w.ws_col;
}

static void display_progress(struct progress* p){
	/* -2 for end brackets */
	int num_blank = get_width() - 3 - strlen("(000.00%)");
	long double pct = (long double)p->count / p->max;
	int num_pound = (uint64_t)(num_blank * pct);
	int num_space = num_blank - num_pound;
	time_t time_tmp = time(NULL);
	int i;

	/* only want to print once per second,
	 * because printf is kind of slow */
	if (time_tmp <= p->time_prev){
		return;
	}
	else{
		p->time_prev = time_tmp;
	}

	/* \r returns to beginning of line */
	printf("\r[");
	for (i = 0; i < num_pound; ++i){
		printf("%c", '#');
	}
	for (i = 0; i < num_space; ++i){
		printf("%c", ' ');
	}
	printf("]");
	printf("(%6.2Lf%%)", pct * 100.0);
	/* needed to actually display chars */
	fflush(stdout);
}

struct progress* start_progress(const char* text, uint64_t max){
	struct progress* p = malloc(sizeof(struct progress));
	if (!p){
		log_enomem();
		return NULL;
	}
	p->text = text;
	p->max = max;
	p->count = 0;
	/* -1 to display progress immediately instead of after
	 * 1 second */
	p->time_prev = time(NULL) - 1;

	if (text){
		/* remove terminal cursor blinking */
		printf("%s\033[?25l\n", p->text);
	}
	else{
		printf("\033[?25l\n");
	}
	display_progress(p);
	return p;
}

void inc_progress(struct progress* p, uint64_t count){
	if (!p){
		return;
	}
	p->count += count;
	display_progress(p);
}

void set_progress(struct progress* p, uint64_t count){
	if (!p){
		return;
	}
	p->count = count;
	display_progress(p);
}

void finish_progress(struct progress* p){
	if (!p){
		return;
	}
	/* 100% progress */
	p->count = p->max;
	/* make sure it displays */
	p->time_prev--;
	/* display final progress */
	display_progress(p);
	/* restore terminal cursor blinking */
	printf("\033[?25h\n");
	free(p);
}
