#include "test_base.h"
#include "../backup.h"
#include "../options/options.h"
#include "../strings/stringarray.h"
#include "../strings/stringhelper.h"
#include "../log.h"
#include "../cloud/keys.h"

void test_backup(enum TEST_STATUS* status){
	char* path = sh_concat_path(sh_getcwd(), "TEST_DIR");
	char* path_exclude = sh_concat_path(sh_getcwd(), "TEST_DIR/excl");
	struct options* opt = NULL;
	char** files = NULL;
	size_t files_len = 0;
	size_t i;
	unsigned char data[1337];

	TEST_ASSERT(path && path_exclude);

	setup_test_environment_full(path, &files, &files_len);

	opt = options_new();
	TEST_ASSERT(opt);

	co_set_username(opt->cloud_options, MEGA_SAMPLE_USERNAME);
	co_set_password(opt->cloud_options, MEGA_SAMPLE_PASSWORD);
	co_set_cp(opt->cloud_options, CLOUD_MEGA);
	sa_add(opt->directories, path);
	sa_add(opt->exclude, path_exclude);
	free(opt->output_directory);
	opt->output_directory = sh_concat_path(sh_getcwd(), "TEST_DIR/backup");
	sa_add(opt->exclude, opt->output_directory);

	TEST_ASSERT(backup(opt) == 0);

	fill_sample_data(data, sizeof(data));
	for (i = 0; i < files_len; i += 2){
		create_file(files[i], data, sizeof(data));
	}

	TEST_ASSERT(backup(opt) == 0);

cleanup:
	cleanup_test_environment("TEST_DIR", files);
	opt ? options_free(opt) : (void)0;
	free(path);
	free(path_exclude);
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_backup)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
