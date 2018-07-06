/* zip_test.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* low buffer length to make sure the file can't be read in one shot */
#define BUFFER_LEN (32)
#include "../test_base.h"
#include "../../log.h"
#include "../../compression/zip.h"
#include <stdlib.h>

void test_compress_gzip(enum TEST_STATUS* status){
	const char* file = "file.txt";
	const char* arch = "file.txt.gz";
	unsigned char data[1337];
	const char* system_cmd = "gzip -d file.txt.gz";

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));
	TEST_ASSERT(zip_compress(file, arch, COMPRESSOR_GZIP, 3, GZIP_NORMAL) == 0);

	remove(file);
	printf("%s\n", system_cmd);
	system(system_cmd);
	TEST_ASSERT(memcmp_file_data(file, data, sizeof(data)) == 0);

cleanup:
	remove(file);
	remove(arch);
}

void test_compress_bzip2(enum TEST_STATUS* status){
	const char* file = "file.txt";
	const char* arch = "file.txt.bz2";
	unsigned char data[1337];
	const char* system_cmd = "bzip2 -d file.txt.bz2";

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));
	TEST_ASSERT(zip_compress(file, arch, COMPRESSOR_BZIP2, 3, BZIP2_NORMAL) == 0);

	remove(file);
	printf("%s\n", system_cmd);
	system(system_cmd);
	TEST_ASSERT(memcmp_file_data(file, data, sizeof(data)) == 0);

cleanup:
	remove(file);
	remove(arch);
}

void test_compress_xz(enum TEST_STATUS* status){
	const char* file = "file.txt";
	const char* arch = "file.txt.xz";
	unsigned char data[1337];
	const char* system_cmd = "xz -d file.txt.xz";

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));
	TEST_ASSERT(zip_compress(file, arch, COMPRESSOR_XZ, 3, XZ_NORMAL) == 0);

	remove(file);
	system(system_cmd);
	printf("%s\n", system_cmd);
	TEST_ASSERT(memcmp_file_data(file, data, sizeof(data)) == 0);

cleanup:
	remove(file);
	remove(arch);
}

void test_compress_lz4(enum TEST_STATUS* status){
	const char* file = "file.txt";
	const char* arch = "file.txt.lz4";
	unsigned char data[1337];
	const char* system_cmd = "lz4 -d file.txt.lz4";

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));
	TEST_ASSERT(zip_compress(file, arch, COMPRESSOR_LZ4, 3, LZ4_NORMAL) == 0);

	remove(file);
	printf("%s\n", system_cmd);
	system(system_cmd);
	TEST_ASSERT(memcmp_file_data(file, data, sizeof(data)) == 0);

cleanup:
	remove(file);
	remove(arch);
}

void test_decompress_gzip(enum TEST_STATUS* status){
	const char* file = "file.txt";
	const char* arch = "file.txt.gz";
	unsigned char data[1337];
	const char* system_cmd = "gzip -3 file.txt";

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));
	printf("%s\n", system_cmd);
	system(system_cmd);

	remove(file);
	TEST_ASSERT(zip_decompress(arch, file, COMPRESSOR_GZIP, GZIP_NORMAL) == 0);
	TEST_ASSERT(memcmp_file_data(file, data, sizeof(data)) == 0);

cleanup:
	remove(file);
	remove(arch);
}

void test_decompress_bzip2(enum TEST_STATUS* status){
	const char* file = "file.txt";
	const char* arch = "file.txt.bz2";
	unsigned char data[1337];
	const char* system_cmd = "bzip2 -3 file.txt";

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));
	printf("%s\n", system_cmd);
	system(system_cmd);

	remove(file);
	TEST_ASSERT(zip_decompress(arch, file, COMPRESSOR_BZIP2, BZIP2_NORMAL) == 0);
	TEST_ASSERT(memcmp_file_data(file, data, sizeof(data)) == 0);

cleanup:
	remove(file);
	remove(arch);
}

void test_decompress_xz(enum TEST_STATUS* status){
	const char* file = "file.txt";
	const char* arch = "file.txt.xz";
	unsigned char data[1337];
	const char* system_cmd = "xz -3 file.txt";

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));
	printf("%s\n", system_cmd);
	system(system_cmd);

	remove(file);
	TEST_ASSERT(zip_decompress(arch, file, COMPRESSOR_XZ, XZ_NORMAL) == 0);
	TEST_ASSERT(memcmp_file_data(file, data, sizeof(data)) == 0);

cleanup:
	remove(file);
	remove(arch);
}

void test_decompress_lz4(enum TEST_STATUS* status){
	const char* file = "file.txt";
	const char* arch = "file.txt.lz4";
	unsigned char data[1337];
	const char* system_cmd = "lz4 -6 file.txt";

	fill_sample_data(data, sizeof(data));
	create_file(file, data, sizeof(data));
	printf("%s\n", system_cmd);
	system(system_cmd);

	remove(file);
	TEST_ASSERT(zip_decompress(arch, file, COMPRESSOR_LZ4, LZ4_NORMAL) == 0);
	TEST_ASSERT(memcmp_file_data(file, data, sizeof(data)) == 0);

cleanup:
	remove(file);
	remove(arch);
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_compress_gzip),
		MAKE_TEST(test_compress_bzip2),
		MAKE_TEST(test_compress_xz),
		MAKE_TEST(test_compress_lz4),
		MAKE_TEST(test_decompress_gzip),
		MAKE_TEST(test_decompress_bzip2),
		MAKE_TEST(test_decompress_xz),
		MAKE_TEST(test_decompress_lz4)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
