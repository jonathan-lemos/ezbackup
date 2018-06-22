#include "test_base.h"
#include "../backup.h"
#include "../log.h"

int test_backup(void){
	return TEST_SUCCESS;
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_backup)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
}
