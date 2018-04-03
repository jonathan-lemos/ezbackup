#include "test_base.h"
#include "../progressbar.h"
#include <unistd.h>

int main(void){
	struct progress* p;

	p = start_progress("Test progress", 100000);
	while (p->count < p->max){
		usleep(66);
		inc_progress(p, 6);
	}
	finish_progress(p);
	return 0;
}
