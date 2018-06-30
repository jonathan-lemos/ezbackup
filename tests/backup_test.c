#include "test_base.h"
#include "../backup.h"
#include "../options/options.h"
#include "../log.h"
#include "../cloud/keys.h"

void test_backup(enum TEST_STATUS* status){
	struct options* opt = NULL;

	opt = options_new();
	TEST_ASSERT(opt);

	co_set_username(opt->cloud_options, MEGA_SAMPLE_USERNAME);
	co_set_password(opt->cloud_options, MEGA_SAMPLE_PASSWORD);
	opt->cloud_options->cp = CLOUD_MEGA;

cleanup:
	opt ? options_free(opt) : (void)0;
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_backup)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
