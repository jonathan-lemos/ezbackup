#include "test_base.h"
#include "../filehelper.h"
#include "../log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

void test_read_file(enum TEST_STATUS* status){
	const char* sample_file = "file1.txt";
	const char* sample_file2 = "file2.txt";
	unsigned char sample_data[4096];
	FILE* fp1 = NULL;
	FILE* fp2 = NULL;
	unsigned char buf[16];
	int len;

	fill_sample_data(sample_data, sizeof(sample_data));

	create_file(sample_file, sample_data, sizeof(sample_data));
	/* copy fp1 to fp2 */
	fp1 = fopen(sample_file, "rb");
	TEST_ASSERT(fp1);
	fp2 = fopen(sample_file2, "wb");
	TEST_ASSERT(fp2);

	/* while the file has data */
	while ((len = read_file(fp1, buf, sizeof(buf))) > 0){
		/* write it to fp2 */
		fwrite(buf, 1, len, fp2);
		TEST_ASSERT(ferror(fp2) == 0);
	}
	/* if len is -1, there was an error */
	TEST_ASSERT(len == 0);

	/* flush fp2's buffer to file */
	TEST_ASSERT(fflush(fp2) == 0);

	/* make sure the two files match */
	TEST_ASSERT(memcmp_file_file(sample_file, sample_file2) == 0);

	/* these throw an error if fclose fails while
	 * the below two lines don't */
	TEST_ASSERT_FREE(fp1, fclose);
	TEST_ASSERT_FREE(fp2, fclose);

cleanup:
	fp1 ? fclose(fp1) : 0;
	fp2 ? fclose(fp2) : 0;
	remove(sample_file);
	remove(sample_file2);
}

void test_temp_fopen(enum TEST_STATUS* status){
	struct TMPFILE* tmp = NULL;
	char* file = NULL;
	unsigned char sample_data[4096];

	fill_sample_data(sample_data, sizeof(sample_data));

	tmp = temp_fopen();
	TEST_ASSERT(tmp);

	/* copy filename for later use */
	file = malloc(strlen(tmp->name) + 1);
	TEST_ASSERT(file);
	strcpy(file, tmp->name);

	TEST_ASSERT(does_file_exist(file));

	fwrite(sample_data, 1, sizeof(sample_data), tmp->fp);
	temp_fflush(tmp);

	/* check that the data we wrote is the same as sample_data */
	TEST_ASSERT(memcmp_file_data(tmp->name, sample_data, sizeof(sample_data)) == 0);

	TEST_FREE(tmp, temp_fclose);

	/* test that the file no longer exists */
	TEST_ASSERT(!does_file_exist(file));

cleanup:
	tmp ? temp_fclose(tmp) : (void)0;
	free(tmp);
	free(file);
}

void test_file_opened_for_reading(enum TEST_STATUS* status){
	const char* sample_file = "file1.txt";
	unsigned char sample_data[4096];
	FILE* fp = NULL;
	size_t i;
	char* modes_accept[] = {
		"r",
		"rb",
		"r+",
		"r+b",
		"w+",
		"w+b"
	};
	char* modes_deny[] = {
		"w",
		"wb",
		"a"
	};

	fill_sample_data(sample_data, sizeof(sample_data));

	for (i = 0; i < sizeof(modes_accept) / sizeof(modes_accept[0]); ++i){
		create_file(sample_file, sample_data, sizeof(sample_data));
		fp = fopen(sample_file, modes_accept[i]);
		TEST_ASSERT(fp);
		TEST_ASSERT(file_opened_for_reading(fp));
		TEST_ASSERT_FREE(fp, fclose);
	}

	for (i = 0; i < sizeof(modes_deny) / sizeof(modes_deny[0]); ++i){
		create_file(sample_file, sample_data, sizeof(sample_data));
		fp = fopen(sample_file, modes_deny[i]);
		TEST_ASSERT(fp);
		TEST_ASSERT(!file_opened_for_reading(fp));
		TEST_ASSERT_FREE(fp, fclose);
	}

cleanup:
	fp ? fclose(fp) : 0;
}

void test_file_opened_for_writing(enum TEST_STATUS* status){
	const char* sample_file = "file1.txt";
	unsigned char sample_data[4096];
	FILE* fp = NULL;
	size_t i;
	char* modes_accept[] = {
		"w",
		"wb",
		"r+",
		"r+b",
		"w+",
		"w+b",
		"a"
	};
	char* modes_deny[] = {
		"r",
		"rb"
	};

	fill_sample_data(sample_data, sizeof(sample_data));

	for (i = 0; i < sizeof(modes_accept) / sizeof(modes_accept[0]); ++i){
		create_file(sample_file, sample_data, sizeof(sample_data));
		fp = fopen(sample_file, modes_accept[i]);
		TEST_ASSERT(fp);
		TEST_ASSERT(file_opened_for_writing(fp));
		TEST_ASSERT_FREE(fp, fclose);
	}

	for (i = 0; i < sizeof(modes_deny) / sizeof(modes_deny[0]); ++i){
		create_file(sample_file, sample_data, sizeof(sample_data));
		fp = fopen(sample_file, modes_deny[i]);
		TEST_ASSERT(fp);
		TEST_ASSERT(!file_opened_for_writing(fp));
		TEST_ASSERT_FREE(fp, fclose);
	}

cleanup:
	fp ? fclose(fp) : 0;
}

void test_get_file_size(enum TEST_STATUS* status){
	const char* sample_file = "file1.txt";
	unsigned char sample_data[1337];
	FILE* fp = NULL;

	fill_sample_data(sample_data, sizeof(sample_data));
	fill_sample_data(sample_data, sizeof(sample_data));

	create_file(sample_file, sample_data, sizeof(sample_data));
	TEST_ASSERT(get_file_size(sample_file) == sizeof(sample_data));

	fp = fopen(sample_file, "rb");
	TEST_ASSERT(fp);

	TEST_ASSERT(get_file_size_fp(fp) == sizeof(sample_data));

cleanup:
	fp ? fclose(fp) : 0;
}

void test_copy_file(enum TEST_STATUS* status){
	const char* sample_file1 = "file1.txt";
	const char* sample_file2 = "file2.txt";
	unsigned char sample_data[4096];

	fill_sample_data(sample_data, sizeof(sample_data));

	create_file(sample_file1, sample_data, sizeof(sample_data));
	copy_file(sample_file1, sample_file2);

	TEST_ASSERT(memcmp_file_file(sample_file1, sample_file2) == 0);

cleanup:
	remove(sample_file1);
	remove(sample_file2);
}

void test_rename_file(enum TEST_STATUS* status){
	const char* sample_file1 = "file1.txt";
	const char* sample_file2 = "file2.txt";
	unsigned char sample_data[4096];

	fill_sample_data(sample_data, sizeof(sample_data));

	create_file(sample_file1, sample_data, sizeof(sample_data));
	rename_file(sample_file1, sample_file2);

	TEST_ASSERT(!does_file_exist(sample_file1));
	TEST_ASSERT(memcmp_file_data(sample_file2, sample_data, sizeof(sample_data)) == 0);

cleanup:
	remove(sample_file1);
	remove(sample_file2);
}

void test_exists(enum TEST_STATUS* status){
	const char* dir = "dir";
	const char* file = "file";
	const unsigned char data[4] = { 't', 'e', 's', 't' };

	create_file(file, data, sizeof(data));
	TEST_ASSERT(mkdir(dir, 0755) == 0);

	TEST_ASSERT(directory_exists(dir));
	TEST_ASSERT(!directory_exists("noexist"));
	TEST_ASSERT(file_exists(file));
	TEST_ASSERT(!file_exists("noexist.txt"));

cleanup:
	rmdir(dir);
	remove(file);
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_read_file),
		MAKE_TEST(test_temp_fopen),
		MAKE_TEST(test_file_opened_for_reading),
		MAKE_TEST(test_file_opened_for_writing),
		MAKE_TEST(test_copy_file),
		MAKE_TEST(test_rename_file),
		MAKE_TEST(test_exists)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
