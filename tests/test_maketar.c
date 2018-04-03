#include "test_base.h"
#include "../maketar.h"
#include "../error.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

const char* const sample_tar = "test.tar";
const char* const sample_file1 = "file1.txt";
const unsigned char sample_data1[] = { 'A', 'B', 'C' };
const char* const sample_file2 = "file2.txt";
const unsigned char sample_data2[] = { 'a', 'y', 'y', 'l', 'm', 'a', 'o' };
const char* const sample_file3 = "file3.txt";
unsigned char sample_data3[1024];

void test_tar_add_file(void){
	TAR* tp;
	char tar_cmd[1024];

	printf_blue("Testing tar_add_file()\n");

	create_file(sample_file1, sample_data1, sizeof(sample_data1));
	create_file(sample_file2, sample_data2, sizeof(sample_data2));
	create_file(sample_file3, sample_data3, sizeof(sample_data3));

	printf_yellow("Calling tar_create()\n");
	tp = tar_create(sample_tar, COMPRESSOR_BZIP2);

	printf_yellow("Calling tar_add_create[_ex])\n");
	massert(tar_add_file_ex(tp, sample_file1, "f-f-fire.txt", 1, "Swiggity swessage, here's the message") == 0);
	massert(tar_add_file(tp, sample_file2) == 0);
	massert(tar_add_file_ex(tp, sample_file3, sample_file3, 1, "msg2") == 0);

	printf_yellow("Calling tar_close()\n");
	massert(tar_close(tp) == 0);

	remove(sample_file1);
	remove(sample_file2);
	remove(sample_file3);

	printf_yellow("Extracting tar\n");
	sprintf(tar_cmd, "tar -xf %s", sample_tar);
	printf("%s\n", tar_cmd);
	system(tar_cmd);

	printf_yellow("Checking that the files match\n");
	massert(memcmp_file_data("f-f-fire.txt", sample_data1, sizeof(sample_data1)) == 0);
	massert(memcmp_file_data(sample_file2, sample_data2, sizeof(sample_data2)) == 0);
	massert(memcmp_file_data(sample_file3, sample_data3, sizeof(sample_data3)) == 0);

	remove("f-f-fire.txt");
	remove(sample_file2);
	remove(sample_file3);
	remove(sample_tar);

	printf_green("Finished testing tar_add_file()\n\n");
}

void test_tar_extract(void){
	char tar_cmd[1024];
	char cwd[4096];
	char path[4096];

	printf_blue("Testing tar_extract()\n");

	massert(getcwd(cwd, sizeof(cwd)) != NULL);

	create_file(sample_file1, sample_data1, sizeof(sample_data1));
	create_file(sample_file2, sample_data2, sizeof(sample_data2));
	create_file(sample_file3, sample_data3, sizeof(sample_data3));

	printf_yellow("Creating tar\n");
	sprintf(tar_cmd, "tar -cjf %s %s %s %s", sample_tar, sample_file1, sample_file2, sample_file3);
	printf("%s\n", tar_cmd);
	system(tar_cmd);

	remove(sample_file1);
	remove(sample_file2);
	remove(sample_file3);

	printf_yellow("Calling tar_extract_file()\n");
	sprintf(path, "%s/%s", cwd, sample_file1);
	massert(tar_extract_file(sample_tar, sample_file1, sample_file1) == 0);

	printf_yellow("Checking that it matches\n");
	massert(memcmp_file_data(sample_file1, sample_data1, sizeof(sample_data1)) == 0);

	printf_yellow("Calling tar_extract()\n");
	massert(tar_extract(sample_tar, cwd) == 0);

	printf_yellow("Checking that all files match\n");
	massert(memcmp_file_data(sample_file1, sample_data1, sizeof(sample_data1)) == 0);
	massert(memcmp_file_data(sample_file2, sample_data2, sizeof(sample_data2)) == 0);
	massert(memcmp_file_data(sample_file3, sample_data3, sizeof(sample_data3)) == 0);

	remove(sample_tar);
	remove(sample_file1);
	remove(sample_file2);
	remove(sample_file3);

	printf_green("Finished testing tar_extract()\n");
}

void test_get_compressor_byname(void){
	massert(get_compressor_byname("none") == COMPRESSOR_NONE);
	massert(get_compressor_byname("gz")   == COMPRESSOR_GZIP);
	massert(get_compressor_byname("bz2")  == COMPRESSOR_BZIP2);
	massert(get_compressor_byname("xz")   == COMPRESSOR_XZ);
	massert(get_compressor_byname("lz4")  == COMPRESSOR_LZ4);
	massert(get_compressor_byname("lmao") == COMPRESSOR_INVALID);
}

void test_compressor_to_string(void){
	massert(strcmp(compressor_to_string(COMPRESSOR_NONE), "none") == 0);
	massert(strcmp(compressor_to_string(COMPRESSOR_GZIP), "gzip") == 0);
	massert(strcmp(compressor_to_string(COMPRESSOR_BZIP2), "bzip2") == 0);
	massert(strcmp(compressor_to_string(COMPRESSOR_XZ), "xz") == 0);
	massert(strcmp(compressor_to_string(COMPRESSOR_LZ4), "lz4") == 0);
	massert(strcmp(compressor_to_string(COMPRESSOR_INVALID), "unknown") == 0);
}

int main(void){
	int i;

	log_setlevel(LEVEL_INFO);
	set_signal_handler();

	for (i = 0; i < (int)sizeof(sample_data3); ++i){
		sample_data3[i] = '0' + i % 10;
	}

	test_tar_add_file();
	test_tar_extract();
	test_get_compressor_byname();
	test_compressor_to_string();

	return 0;
}
