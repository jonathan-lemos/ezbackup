#include "test_base.h"
#include "../filehelper.h"
#include <stdlib.h>
#include <string.h>

int test_read_file(void){
	const char* sample_file = "file1.txt";
	const char* sample_file2 = "file2.txt";
	unsigned char sample_data[4096];
	FILE* fp1;
	FILE* fp2;
	unsigned char buf[16];
	int len;
	size_t i;

	for (i = 0; i < sizeof(sample_data); ++i){
		sample_data[i] = i % 10 + '0';
	}

	create_file(sample_file, sample_data, sizeof(sample_data));
	fp1 = fopen(sample_file, "rb");
	TEST_ASSERT(fp1);
	fp2 = fopen(sample_file2, "wb");
	TEST_ASSERT(fp2);

	while ((len = read_file(fp1, buf, sizeof(buf))) > 0){
		fwrite(buf, 1, len, fp2);
		TEST_ASSERT(ferror(fp2) == 0);
	}
	TEST_ASSERT(len == 0);

	TEST_ASSERT(fflush(fp2) == 0);

	TEST_ASSERT(memcmp_file_file(sample_file, sample_file2) == 0);

	TEST_ASSERT(fclose(fp1) == 0);
	TEST_ASSERT(fclose(fp2) == 0);

	remove(sample_file);
	remove(sample_file2);

	printf_green("Finsihed testing read_file()\n\n");
}

void test_temp_fopen(void){
	struct TMPFILE* tmp;
	char tmp_template[] = "tmp_XXXXXX";

	printf_blue("Testing temp_fopen()\n");

	printf_yellow("Calling temp_fopen()\n");
	tmp = temp_fopen();
	TEST_ASSERT(tmp);

	printf_yellow("Checking that the file exists\n");
	TEST_ASSERT(does_file_exist(tmp_template));

	printf_yellow("Writing to file\n");
	fwrite(sample_data, 1, sizeof(sample_data), tmp->fp);
	temp_fflush(tmp);

	printf_yellow("Verifying file integrity\n");
	TEST_ASSERT(memcmp_file_data(tmp_template, sample_data, sizeof(sample_data)) == 0);

	temp_fclose(tmp);

	printf_green("Finished testing temp_fopen()\n\n");
}

void test_file_opened_for_reading(void){
	FILE* fp;
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

	printf_blue("Testing file_opened_for_reading()\n");

	for (i = 0; i < sizeof(modes_accept) / sizeof(modes_accept[0]); ++i){
		create_file(sample_file, sample_data, sizeof(sample_data));
		printf_yellow("Calling file_opened_for_reading(\"%s\")\n", modes_accept[i]);
		fp = fopen(sample_file, modes_accept[i]);
		TEST_ASSERT(fp);
		TEST_ASSERT(file_opened_for_reading(fp));
		TEST_ASSERT(fclose(fp) == 0);
	}

	for (i = 0; i < sizeof(modes_deny) / sizeof(modes_deny[0]); ++i){
		create_file(sample_file, sample_data, sizeof(sample_data));
		printf_yellow("Calling file_opened_for_reading(\"%s\")\n", modes_deny[i]);
		fp = fopen(sample_file, modes_deny[i]);
		TEST_ASSERT(fp);
		TEST_ASSERT(!file_opened_for_reading(fp));
		TEST_ASSERT(fclose(fp) == 0);
	}

	printf_green("Finished testing file_opened_for_reading()\n\n");
}

void test_file_opened_for_writing(void){
	FILE* fp;
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

	printf_blue("Testing file_opened_for_writing()\n");

	for (i = 0; i < sizeof(modes_accept) / sizeof(modes_accept[0]); ++i){
		create_file(sample_file, sample_data, sizeof(sample_data));
		printf_yellow("Calling file_opened_for_writing(\"%s\")\n", modes_accept[i]);
		fp = fopen(sample_file, modes_accept[i]);
		TEST_ASSERT(fp);
		TEST_ASSERT(file_opened_for_writing(fp));
		TEST_ASSERT(fclose(fp) == 0);
	}

	for (i = 0; i < sizeof(modes_deny) / sizeof(modes_deny[0]); ++i){
		create_file(sample_file, sample_data, sizeof(sample_data));
		printf_yellow("Calling file_opened_for_writing(\"%s\")\n", modes_deny[i]);
		fp = fopen(sample_file, modes_deny[i]);
		TEST_ASSERT(fp);
		TEST_ASSERT(!file_opened_for_writing(fp));
		TEST_ASSERT(fclose(fp) == 0);
	}

	printf_green("Finished testing file_opened_for_writing()\n\n");
}

int main(void){
	size_t i;

	set_signal_handler();
	for (i = 0; i < sizeof(sample_data); ++i){
		sample_data[i] = i % 10 + '0';
	}

	test_read_file();
	test_temp_fopen();
	test_file_opened_for_reading();
	test_file_opened_for_writing();
	return 0;
}
