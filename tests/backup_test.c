#include "test_base.h"
#include "../backup.h"
#include "../log.h"
#include <errno.h>
#include <sys/resource.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

int test_backup(void){
	return TEST_SUCCESS;
}

int main(void){
	struct unit_test tests[] = {
		UNIT_TEST(test_backup)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
}
