#include "test_base.h"
#include "../backup.h"
#include "../log.h"

void test_backup(enum TEST_STATUS* status){
	TEST_ASSERT(1);
cleanup:
	;
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_backup)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
}
