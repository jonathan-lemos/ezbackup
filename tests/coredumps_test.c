#include "test_base.h"
#include "../coredumps.h"
#include "../log.h"
#include <sys/resource.h>

void test_coredumps(enum TEST_STATUS* status){
	struct rlimit rl;

	disable_core_dumps();
	TEST_ASSERT(getrlimit(RLIMIT_CORE, &rl) == 0);
	TEST_ASSERT(rl.rlim_max == 0 && rl.rlim_cur == 0);

	enable_core_dumps();
	TEST_ASSERT(getrlimit(RLIMIT_CORE, &rl) == 0);
	TEST_ASSERT(rl.rlim_max > 0 && rl.rlim_cur > 0);

cleanup:
	;
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_coredumps)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
