#include "test_base.h"
#include "../coredumps.h"
#include "../error.h"

int main(void){
	set_signal_handler();
	log_setlevel(LEVEL_INFO);

	printf_blue("Testing disable_core_dumps()\n");
	disable_core_dumps();
	printf_blue("Testing enable_core_dumps()\n");
	enable_core_dumps();

	return 0;
}
