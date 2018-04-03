#include "test_base.h"
#include "../error.h"

int main(void){
	LOG_LEVEL level;
	set_signal_handler();

	for (level = LEVEL_NONE; level <= LEVEL_INFO; level++){
		log_setlevel(level);
		log_info(__FL__, "info");
		log_debug(__FL__, "debug");
		log_warning(__FL__, "warning");
		log_error(__FL__, "error");
		log_fatal(__FL__, "fatal");
		printf("\n");
	}

	return 0;
}
