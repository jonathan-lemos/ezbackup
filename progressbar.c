/* prototypes */
#include "progressbar.h"
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

static void display_progress(progress* p){
	/* no idea why -4, but it works */
	int num_blank = get_width() - 4 - strlen("(000.00%)");
	double pct = (double)p->count / p->max;
	int num_pound = (int)(num_blank * pct) + 1;
	int num_space = num_blank - num_pound;

	/* \r returns to beginning of line */

	printf("\r[");
	for (; num_pound >= 0; --num_pound){
		printf("%c", '#');
	}
	for (; num_space >= 0; --num_space){
		printf("%c", ' ');
	}
	printf("]");
	printf("(%5.2f%%)", pct * 100.0);
	/* needed to actually display chars */
	fflush(stdout);
}

progress* start_progress(const char* text, int max){
	progress* p = malloc(sizeof(progress));
	if (!p){
		return NULL;
	}
	p->text = text;
	p->max = max;
	p->count = 0;

	if (text){
		printf("%s\033[?25l\n", p->text);
	}
	else{
		printf("\033[?25l\n");
	}
	return p;
}

void inc_progress(progress* p, int count){
	if (!p){
		return;
	}
	p->count += count;
	display_progress(p);
}

void set_progress(progress* p, int count){
	if (!p){
		return;
	}
	p->count = count;
	display_progress(p);
}

void finish_progress(progress* p){
	if (!p){
		return;
	}
	p->count = p->max;
	display_progress(p);
	printf("\033[?25h\n");
	free(p);
}
