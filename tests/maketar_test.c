/* test_maketar.c -- tests maketar.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "test_base.h"
#include "../maketar.h"
#include "../log.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static void fill_sample_data(unsigned char* sample_data, size_t len){
	size_t i;

	for (i = 0; i < len; ++i){
		sample_data[i] = i % 10 + '0';
	}
}

void test_tar_add_file(enum TEST_STATUS* status){
	const char* sample_tar = "sample.tar";
	const char* sample_file1 = "file1.txt";
	const char* sample_file2 = "file2.txt";
	const char* sample_file3 = "file3.txt";
	unsigned char sample_data1[] = {'t', 'e', 's', 't'};
	unsigned char sample_data2[] = {'a', 'b', 'c', 'd'};
	unsigned char sample_data3[4096];
	TAR* tp;
	char tar_cmd[1024];

	fill_sample_data(sample_data3, sizeof(sample_data3));

	create_file(sample_file1, sample_data1, sizeof(sample_data1));
	create_file(sample_file2, sample_data2, sizeof(sample_data2));
	create_file(sample_file3, sample_data3, sizeof(sample_data3));

	tp = tar_create(sample_tar, COMPRESSOR_BZIP2, 0);

	/* add sample_file1 as f-f-fire.txt */
	TEST_ASSERT(tar_add_file_ex(tp, sample_file1, "f-f-fire.txt", 1, "Swiggity swessage, here's the message") == 0);
	TEST_ASSERT(tar_add_file(tp, sample_file2) == 0);
	TEST_ASSERT(tar_add_file_ex(tp, sample_file3, sample_file3, 1, "msg2") == 0);

	TEST_ASSERT(tar_close(tp) == 0);

	remove(sample_file1);
	remove(sample_file2);
	remove(sample_file3);

	/* system command to extract tar */
	sprintf(tar_cmd, "tar -xf %s", sample_tar);
	printf("%s\n", tar_cmd);
	system(tar_cmd);

	/* check that the extracted files match up with their originals */
	TEST_ASSERT(memcmp_file_data("f-f-fire.txt", sample_data1, sizeof(sample_data1)) == 0);
	TEST_ASSERT(memcmp_file_data(sample_file2, sample_data2, sizeof(sample_data2)) == 0);
	TEST_ASSERT(memcmp_file_data(sample_file3, sample_data3, sizeof(sample_data3)) == 0);

cleanup:
	remove("f-f-fire.txt");
	remove(sample_file1);
	remove(sample_file2);
	remove(sample_file3);
	remove(sample_tar);
}

void test_tar_extract(enum TEST_STATUS* status){
	const char* sample_tar = "sample.tar";
	const char* sample_file1 = "file1.txt";
	const char* sample_file2 = "file2.txt";
	const char* sample_file3 = "file3.txt";
	unsigned char sample_data1[] = {'t', 'e', 's', 't'};
	unsigned char sample_data2[] = {'a', 'b', 'c', 'd'};
	unsigned char sample_data3[4096];
	TAR* tp;

	fill_sample_data(sample_data3, sizeof(sample_data3));

	create_file(sample_file1, sample_data1, sizeof(sample_data1));
	create_file(sample_file2, sample_data2, sizeof(sample_data2));
	create_file(sample_file3, sample_data3, sizeof(sample_data3));

	tp = tar_create(sample_tar, COMPRESSOR_BZIP2, 0);

	/* add sample_file1 as f-f-fire.txt */
	TEST_ASSERT(tar_add_file_ex(tp, sample_file1, "f-f-fire.txt", 1, "Swiggity swessage, here's the message") == 0);
	TEST_ASSERT(tar_add_file(tp, sample_file2) == 0);
	TEST_ASSERT(tar_add_file_ex(tp, sample_file3, sample_file3, 1, "msg2") == 0);

	TEST_ASSERT(tar_close(tp) == 0);

	remove(sample_file1);
	remove(sample_file2);
	remove(sample_file3);

	TEST_ASSERT(tar_extract_file(sample_tar, "f-f-fire.txt", "output.txt") == 0);
	TEST_ASSERT(memcmp_file_data("output.txt", sample_data1, sizeof(sample_data1)) == 0);
	remove("output.txt");

	TEST_ASSERT(tar_extract(sample_tar, ".") == 0);

	/* check that the extracted files match up with their originals */
	TEST_ASSERT(memcmp_file_data("f-f-fire.txt", sample_data1, sizeof(sample_data1)) == 0);
	TEST_ASSERT(memcmp_file_data(sample_file2, sample_data2, sizeof(sample_data2)) == 0);
	TEST_ASSERT(memcmp_file_data(sample_file3, sample_data3, sizeof(sample_data3)) == 0);

cleanup:
	remove("f-f-fire.txt");
	remove("output.txt");
	remove(sample_file1);
	remove(sample_file2);
	remove(sample_file3);
	remove(sample_tar);
}


void test_get_compressor_byname(enum TEST_STATUS* status){
	TEST_ASSERT(get_compressor_byname("none") == COMPRESSOR_NONE);
	TEST_ASSERT(get_compressor_byname("gz")   == COMPRESSOR_GZIP);
	TEST_ASSERT(get_compressor_byname("bz2")  == COMPRESSOR_BZIP2);
	TEST_ASSERT(get_compressor_byname("xz")   == COMPRESSOR_XZ);
	TEST_ASSERT(get_compressor_byname("lz4")  == COMPRESSOR_LZ4);
	TEST_ASSERT(get_compressor_byname("lmao") == COMPRESSOR_INVALID);
cleanup:
	;
}

void test_compressor_to_string(enum TEST_STATUS* status){
	TEST_ASSERT(strcmp(compressor_to_string(COMPRESSOR_NONE), "none") == 0);
	TEST_ASSERT(strcmp(compressor_to_string(COMPRESSOR_GZIP), "gzip") == 0);
	TEST_ASSERT(strcmp(compressor_to_string(COMPRESSOR_BZIP2), "bzip2") == 0);
	TEST_ASSERT(strcmp(compressor_to_string(COMPRESSOR_XZ), "xz") == 0);
	TEST_ASSERT(strcmp(compressor_to_string(COMPRESSOR_LZ4), "lz4") == 0);
	TEST_ASSERT(strcmp(compressor_to_string(COMPRESSOR_INVALID), "unknown") == 0);
cleanup:
	;
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_tar_add_file),
		MAKE_TEST(test_tar_extract),
		MAKE_TEST(test_get_compressor_byname),
		MAKE_TEST(test_compressor_to_string)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
