#include "test_base.h"
#include "../readfile.h"
#include <stdlib.h>
#include <string.h>

const char* const sample_file = "test.txt";
const char* const sample_file2 = "test2.txt";
unsigned char sample_data[1024];

void test_read_file(void){
	FILE* fp1;
	FILE* fp2;
	unsigned char buf[16];
	int len;

	printf_blue("Testing read_file()\n");

	create_file(sample_file, sample_data, sizeof(sample_data));
	fp1 = fopen(sample_file, "wb");
	massert(fp1);
	fp2 = fopen(sample_file2, "wb");

	printf_yellow("Calling read_file()\n");
	while ((len = read_file(fp1, buf, sizeof(buf))) > 0){
		fwrite(buf, 1, len, fp2);
		massert(ferror(fp2) == 0);
	}
	massert(len == 0);

	printf_yellow("Verifying that the files match\n");
	massert(memcmp_file_file(sample_file, sample_file2) == 0);

	massert(fclose(fp1) == 0);
	massert(fclose(fp2) == 0);
	remove(sample_file);
	remove(sample_file2);

	printf_green("Finsihed testing read_file()\n\n");
}

void test_temp_fopen(void){
	struct TMPFILE* tmp;
	char* file;

	printf_blue("Testing temp_fopen()\n");

	printf_yellow("Calling temp_fopen()\n");
	tmp = temp_fopen("file_XXXXXX", "wb");
	massert(tmp);

	printf_yellow("Checking that the file exists\n");
	massert(does_file_exist(tmp->name));

	printf_yellow("Writing to file\n");
	fwrite(sample_data, 1, sizeof(sample_data), tmp->fp);
	massert(ferror(tmp->fp) == 0);

	printf_yellow("Verifying file integrity\n");
	massert(memcmp_file_data(tmp->name, sample_data, sizeof(sample_data)) == 0);

	file = malloc(strlen(tmp->name) + 1);
	massert(file);
	strcpy(file, tmp->name);

	printf_yellow("Calling temp_fclose()\n");
	massert(temp_fclose(tmp) == 0);

	printf_yellow("Checking that the file does not exist\n");
	massert(does_file_exist(file) == 0);

	free(file);
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
		massert(fp);
		massert(file_opened_for_reading(fp));
		massert(fclose(fp) == 0);
	}

	for (i = 0; i < sizeof(modes_deny) / sizeof(modes_deny[0]); ++i){
		create_file(sample_file, sample_data, sizeof(sample_data));
		printf_yellow("Calling file_opened_for_reading(\"%s\")\n", modes_deny[i]);
		fp = fopen(sample_file, modes_deny[i]);
		massert(fp);
		massert(!file_opened_for_reading(fp));
		massert(fclose(fp) == 0);
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
		massert(fp);
		massert(file_opened_for_writing(fp));
		massert(fclose(fp) == 0);
	}

	for (i = 0; i < sizeof(modes_deny) / sizeof(modes_deny[0]); ++i){
		create_file(sample_file, sample_data, sizeof(sample_data));
		printf_yellow("Calling file_opened_for_writing(\"%s\")\n", modes_deny[i]);
		fp = fopen(sample_file, modes_deny[i]);
		massert(fp);
		massert(!file_opened_for_writing(fp));
		massert(fclose(fp) == 0);
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
}
