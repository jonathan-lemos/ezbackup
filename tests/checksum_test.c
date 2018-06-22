/* test_checksum.c -- tests checksum.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "../checksum.h"
#include "../checksumsort.h"
#include "../log.h"
#include "test_base.h"
#include <stdlib.h>
#include <string.h>

static const char* const sample_file = "test.txt";
static const unsigned char sample_data[] = {'t', 'e', 's', 't'};
static const unsigned char sample_sha1[] = { 0xA9, 0x4A, 0x8F, 0xE5, 0xCC, 0xB1, 0x9B, 0xA6, 0x1C, 0x4C, 0x08, 0x73, 0xD3, 0x91, 0xE9, 0x87, 0x98, 0x2F, 0xBB, 0xD3 };
static const char sample_sha1_str[] = "A94A8FE5CCB19BA61C4C0873D391E987982FBBD3";

/* bytes_to_hex() */
int test_bytes_to_hex(void){
	char* out;

	TEST_ASSERT(bytes_to_hex(sample_sha1, sizeof(sample_sha1), &out) == 0);
	TEST_ASSERT(strcmp(out, sample_sha1_str) == 0);

	free(out);
	return TEST_SUCCESS;
}

/* checksum() */
int test_checksum(void){
	unsigned char* out;
	unsigned len;

	create_file(sample_file, sample_data, sizeof(sample_data));

	/* hash that file and check it against the expected value */
	TEST_ASSERT(checksum(sample_file, EVP_sha1(), &out, &len) == 0);
	TEST_ASSERT(memcmp(sample_sha1, out, len) == 0);

	free(out);
	remove(sample_file);
	return TEST_SUCCESS;
}

/* add_checksum_to_file()
 * sort_checksum_file()
 * add_checksum_to_file() with previous file */
int test_sort_checksum_file(void){
	FILE* fp1;
	const char* fp1str = "checksum1.txt";
	FILE* fp2;
	const char* fp2str = "checksum2.txt";
	struct element* e1 = NULL;
	struct element* e2 = NULL;

	const char* path = "TEST_ENVIRONMENT";
	char** files;
	size_t files_len;

	size_t i;

	setup_test_environment_basic(path, &files, &files_len);

	/* make output file */
	fp1 = fopen(fp1str, "wb");
	TEST_ASSERT(fp1);

	/* initial checksum run */
	for (i = 1; i < files_len; i += 2){
		int res;
		res = add_checksum_to_file(files[i], EVP_sha1(), fp1, NULL);
		TEST_ASSERT(res >= 0);
		if (res == 1){
			printf("Old element: %s\n", files[i]);
		}
	}
	for (i = 0; i < files_len; i += 2){
		int res;
		res = add_checksum_to_file(files[i], EVP_sha1(), fp1, NULL);
		TEST_ASSERT(res >= 0);
		if (res == 1){
			printf("Old element: %s\n", files[i]);
		}
	}
	TEST_ASSERT(fclose(fp1) == 0);

	/* sorting file */
	TEST_ASSERT(sort_checksum_file(fp1str, fp2str) == 0);
	/* checking that it's sorted */
	fp2 = fopen(fp2str, "rb");
	TEST_ASSERT(fp2);
	e1 = get_next_checksum_element(fp2);
	for (i = 0; i < files_len - 1; ++i){
		e2 = get_next_checksum_element(fp2);
		TEST_ASSERT(strcmp(e1->file, e2->file) <= 0);
		free_element(e1);
		e1 = e2;
	}
	free_element(e2);

	/* moving sorted list to fp1 */
	TEST_ASSERT(fclose(fp2) == 0);
	remove(fp1str);
	rename(fp2str, fp1str);
	fp1 = fopen(fp1str, "rb");
	TEST_ASSERT(fp1);
	fp2 = fopen(fp2str, "wb");
	TEST_ASSERT(fp2);

	/* changing data */
	for (i = 0; i < files_len; i += 2){
		const unsigned char data[] = {'c', 'h', 'a', 'n', 'g', 'e', 'd'};
		create_file(files[i], data, sizeof(data));
	}

	/* checksum run with previous file */
	for (i = 0; i < files_len; ++i){
		int res;
		res = add_checksum_to_file(files[i], EVP_sha1(), fp2, fp1);
		TEST_ASSERT(res >= 0);
		if (res == 1){
			printf("Old element: %s\n", files[i]);
		}
	}
	TEST_ASSERT(fclose(fp1) == 0);
	TEST_ASSERT(fclose(fp2) == 0);

	/* cleaning up */
	cleanup_test_environment(path, files);
	remove(fp1str);
	remove(fp2str);
	return TEST_SUCCESS;
}

int test_search_for_checksum(void){
	char files[100][64];
	char data[100][64];
	FILE* fp1;
	const char* fp1str = "checksum1.txt";
	FILE* fp2;
	const char* fp2str = "checksum2.txt";
	struct element* e1 = NULL;
	struct element* e2 = NULL;
	char* checksum;
	int i;

	/* predictable shuffle */
	srand(0);

	/* make output file */
	fp1 = fopen(fp1str, "wb");
	TEST_ASSERT(fp1);
	/* make files 0 to 100 */
	for (i = 0; i < 100; ++i){
		sprintf(files[i], "file%03d.txt", i);
		sprintf(data[i], "test%03d", i);
	}
	/* shuffle the list */
	for (i = 0; i < 100; ++i){
		char buf[64];
		int index = rand() % 100;
		strcpy(buf, files[i]);
		strcpy(files[i], files[index]);
		strcpy(files[index], buf);
	}
	/* add data to each file */
	for (i = 0; i < 100; ++i){
		create_file(files[i], (unsigned char*)data[i], strlen(data[i]));
	}
	/* initial checksum run */

	for (i = 0; i < 100; ++i){
		int res;
		res = add_checksum_to_file(files[i], EVP_sha1(), fp1, NULL);
		TEST_ASSERT(res >= 0);
		if (res == 1){
			printf("Old element: %s\n", files[i]);
		}
	}
	create_file(sample_file, sample_data, sizeof(sample_data));
	add_checksum_to_file(sample_file, EVP_sha1(), fp1, NULL);

	TEST_ASSERT(fclose(fp1) == 0);
	/* sorting file */

	TEST_ASSERT(sort_checksum_file(fp1str, fp2str) == 0);
	TEST_ASSERT(fclose(fp1) == 0);
	/* checking that it's sorted */
	fp2 = fopen(fp2str, "rb");
	TEST_ASSERT(fp2);
	e1 = get_next_checksum_element(fp2);
	for (i = 0; i < 100 - 1; ++i){
		e2 = get_next_checksum_element(fp2);
		TEST_ASSERT(strcmp(e1->file, e2->file) <= 0);
		free_element(e1);
		e1 = e2;
	}
	free_element(e2);

	TEST_ASSERT(search_for_checksum(fp2, "noexist", &checksum) > 0);
	TEST_ASSERT(search_for_checksum(fp2, sample_file, &checksum) == 0);
	TEST_ASSERT(strcmp(sample_sha1_str, checksum) == 0);

	/* cleaning up */
	free(checksum);
	remove(sample_file);
	for (i = 0; i < 100; ++i){
		remove(files[i]);
	}
	remove(fp1str);
	remove(fp2str);
	return TEST_SUCCESS;
}

int test_create_removed_list(void){
	char files[100][64];
	char data[100][64];
	FILE* fp1;
	const char* fp1str = "checksum1.txt";
	FILE* fp2;
	const char* fp2str = "checksum2.txt";
	char* tmp;
	int ctr = 0;

	int i;

	/* make output file */
	fp1 = fopen(fp1str, "wb");
	TEST_ASSERT(fp1);
	/* make files 0 to 100 */
	for (i = 0; i < 100; ++i){
		sprintf(files[i], "file%03d.txt", i);
		sprintf(data[i], "test%03d", i);
	}
	/* add data to each file */
	for (i = 0; i < 100; ++i){
		create_file(files[i], (unsigned char*)data[i], strlen(data[i]));
	}

	/* initial checksum run */

	for (i = 0; i < 100; ++i){
		int res;
		res = add_checksum_to_file(files[i], EVP_sha1(), fp1, NULL);
		TEST_ASSERT(res >= 0);
		if (res == 1){
			printf("Old element: %s\n", files[i]);
		}
	}

	TEST_ASSERT(fclose(fp1) == 0);
	fp1 = fopen(fp1str, "rb");
	TEST_ASSERT(fp1);
	fp2 = fopen(fp2str, "wb");
	TEST_ASSERT(fp2);

	for (i = 0; i < 100; i += 9){
		remove(files[i]);
	}

	TEST_ASSERT(fclose(fp1) == 0);
	TEST_ASSERT(fclose(fp2) == 0);
	TEST_ASSERT(create_removed_list(fp1str, fp2str) == 0);
	fp2 = fopen(fp2str, "rb");
	TEST_ASSERT(fp2);

	while ((tmp = get_next_removed(fp2)) != NULL){
		int buf;
		TEST_ASSERT(sscanf(tmp, "file%03d.txt", &buf) == 1);
		TEST_ASSERT(buf % 9 == 0);
		ctr++;
		free(tmp);
	}
	TEST_ASSERT(ctr == 12);

	remove(fp1str);
	remove(fp2str);
	for (i = 0; i < 100; ++i){
		remove(files[i]);
	}
	return TEST_SUCCESS;
}

int main(void){
	set_signal_handler();
	log_setlevel(LEVEL_INFO);

	test_bytes_to_hex();
	test_checksum();
	test_sort_checksum_file();
	test_create_removed_list();
	return 0;
}
