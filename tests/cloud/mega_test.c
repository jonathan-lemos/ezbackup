#include "../test_base.h"
#include "../../cloud/mega.h"
#include "../../cloud/keys.h"
#include "../../log.h"
#include <string.h>
#include <time.h>

const char* const sample_file = "test.txt";
const char* const sample_file2 = "test2.txt";
const char* const sample_file3 = "test3.txt";
const unsigned char sample_data[] = "pizza";
const unsigned char sample_data2[] = "avocado";

void test_MEGAdownload(enum TEST_STATUS* status){
	MEGAhandle* mh;
	char buf[256];
	struct file_node** fn;
	size_t fn_len;
	size_t fn_len_prev;
	size_t i;

	create_file(sample_file, sample_data, strlen((const char*)sample_data));
	create_file(sample_file2, sample_data2, strlen((const char*)sample_data2));

	TEST_ASSERT(MEGAlogin(MEGA_SAMPLE_USERNAME, MEGA_SAMPLE_PASSWORD, &mh) == 0);

	TEST_ASSERT(MEGAmkdir("/test1", mh) >= 0);

	TEST_ASSERT(MEGAupload(sample_file, "/test1", "Upload test 1", mh) == 0);
	TEST_ASSERT(MEGAupload(sample_file2, "/test1", "Upload test 2", mh) == 0);

	TEST_ASSERT(MEGAreaddir("/test1", &fn, &fn_len, mh) == 0);
	for (i = 0; i < fn_len; ++i){
		struct tm time;
		char buf[256];

		memcpy(&time, localtime(&(fn[i]->time)), sizeof(struct tm));
		strftime(buf, sizeof(buf), "%c", &time);
		printf("%s (%s)\n", fn[i]->name, buf);
	}

	sprintf(buf, "/test1/%s", sample_file2);
	TEST_ASSERT(MEGArm(buf, mh) == 0);

	fn_len_prev = fn_len;
	TEST_ASSERT(MEGAreaddir("/test1", &fn, &fn_len, mh) == 0);
	TEST_ASSERT(fn_len == fn_len_prev - 1);

	sprintf(buf, "/test1/%s", sample_file);
	printf("%s\n", buf);
	TEST_ASSERT(MEGAdownload(buf, sample_file3, "Downloading file", mh) == 0);
	TEST_ASSERT(memcmp_file_file(sample_file, sample_file3) == 0);

	TEST_ASSERT(MEGArm("/test1", mh) == 0);

	TEST_ASSERT_FREE(mh, MEGAlogout);

cleanup:
	mh ? MEGAlogout(mh) : 0;
	remove(sample_file);
	remove(sample_file2);
	remove(sample_file3);
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_MEGAdownload)
	};
	log_setlevel(LEVEL_INFO);

	START_TESTS(tests);
	return 0;
}
