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
void test_bytes_to_hex(enum TEST_STATUS* status){
	char* out;

	/* sample_sha1 to hex str should equal sample_sha1_str */
	TEST_ASSERT(bytes_to_hex(sample_sha1, sizeof(sample_sha1), &out) == 0);
	TEST_ASSERT(strcmp(out, sample_sha1_str) == 0);

cleanup:
	free(out);
}

/* checksum() */
void test_checksum(enum TEST_STATUS* status){
	unsigned char* out;
	unsigned len;

	create_file(sample_file, sample_data, sizeof(sample_data));

	/* sha1(sample_file) should equal sample_sha1 */
	TEST_ASSERT(checksum(sample_file, EVP_sha1(), &out, &len) == 0);
	TEST_ASSERT(memcmp(sample_sha1, out, len) == 0);

cleanup:
	free(out);
	remove(sample_file);
}

/* add_checksum_to_file()
 * sort_checksum_file()
 * add_checksum_to_file() with previous file */
void test_sort_checksum_file(enum TEST_STATUS* status){
	FILE* fp1 = NULL;
	const char* fp1str = "checksum1.txt";
	FILE* fp2 = NULL;
	const char* fp2str = "checksum2.txt";
	struct element* e1 = NULL;
	struct element* e2 = NULL;
	int n_changed = 0;
	int n_ctr = 0;

	const char* path = "TEST_ENVIRONMENT";
	char** files = NULL;
	size_t files_len = 0;

	size_t i;

	setup_test_environment_basic(path, &files, &files_len);

	fp1 = fopen(fp1str, "wb");
	TEST_ASSERT(fp1);

	/* these two for loops add checkums to file in an unsorted order
	 * this is so we can be sure that sort_checksum_file() actually did something */
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
	TEST_ASSERT_FREE(fp1, fclose);

	TEST_ASSERT(sort_checksum_file(fp1str, fp2str) == 0);

	/* checking that the file is properly sorted */
	fp2 = fopen(fp2str, "rb");
	TEST_ASSERT(fp2);
	/* e1 = previous element, e2 = current element
	 * e2->file should always be greater than e1->file */
	e1 = get_next_checksum_element(fp2);
	for (i = 0; i < files_len - 1; ++i){
		e2 = get_next_checksum_element(fp2);
		TEST_ASSERT(strcmp(e2->file, e1->file) > 0);
		TEST_FREE(e1, free_element);
		e1 = e2;
		e2 = NULL;
	}
	TEST_FREE(e1, free_element);

	/* from this point on we are checking if the incremental checksum update works properly */

	/* moving sorted list to fp1 */
	TEST_ASSERT_FREE(fp2, fclose);
	remove(fp1str);
	rename(fp2str, fp1str);
	fp1 = fopen(fp1str, "rb");
	TEST_ASSERT(fp1);
	fp2 = fopen(fp2str, "wb");
	TEST_ASSERT(fp2);

	/* change every other file */
	for (i = 0; i < files_len; i += 2){
		const unsigned char data[] = {'c', 'h', 'a', 'n', 'g', 'e', 'd'};
		create_file(files[i], data, sizeof(data));
		n_changed++;
	}

	/* check if add_checksum_to_file() skips the unchanged files like it should */
	for (i = 0; i < files_len; ++i){
		int res;
		res = add_checksum_to_file(files[i], EVP_sha1(), fp2, fp1);
		/* less than zero means an error occured */
		TEST_ASSERT(res >= 0);
		/* if file was unchanged */
		if (res == 1){
			printf("Old element: %s\n", files[i]);
		}
		/* file was changed */
		else{
			n_ctr++;
		}
	}
	/* n_changed should equal the number of checksums actually added */
	TEST_ASSERT(n_ctr == n_changed);

cleanup:
	e1 ? free_element(e1) : (void)0;
	e2 ? free_element(e2) : (void)0;
	fp1 ? fclose(fp1) : 0;
	fp2 ? fclose(fp2) : 0;
	cleanup_test_environment(path, files);
	remove(fp1str);
	remove(fp2str);
}

void test_search_for_checksum(enum TEST_STATUS* status){
	FILE* fp1 = NULL;
	const char* fp1str = "checksum1.txt";
	FILE* fp2 = NULL;
	const char* fp2str = "checksum2.txt";
	struct element* e1 = NULL;
	struct element* e2 = NULL;
	char* checksum = NULL;

	const char* path = "TEST_ENVIRONMENT";
	char** files;
	size_t files_len = 0;

	size_t i;

	setup_test_environment_basic(path, &files, &files_len);

	fp1 = fopen(fp1str, "wb");
	TEST_ASSERT(fp1);

	/* these two for loops add checkums to file in an unsorted order
	 * this is so we can be sure that sort_checksum_file() actually did something */
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
	/* add our file to search for right at the end,
	 * since binsearch starts at the middle, we don't want to give it an unfair advantage */
	create_file(sample_file, sample_data, sizeof(sample_data));
	add_checksum_to_file(sample_file, EVP_sha1(), fp1, NULL);

	TEST_ASSERT_FREE(fp1, fclose);

	TEST_ASSERT(sort_checksum_file(fp1str, fp2str) == 0);

	/* checking that the file was properly sorted
	 * file must be sorted for the binary search to work properly */
	fp2 = fopen(fp2str, "rb");
	TEST_ASSERT(fp2);
	e1 = get_next_checksum_element(fp2);
	for (i = 0; i < files_len - 1; ++i){
		e2 = get_next_checksum_element(fp2);
		TEST_ASSERT(e1);
		TEST_ASSERT(e2);
		TEST_ASSERT(strcmp(e1->file, e2->file) <= 0);
		TEST_FREE(e1, free_element);
		e1 = e2;
		e2 = NULL;
	}
	TEST_FREE(e1, free_element);

	/* search_for_checksum returns -1 on error, 1 on not found, 0 on success */
	TEST_ASSERT(search_for_checksum(fp2, "noexist", &checksum) > 0);
	/* check that it found our file and the checksums match */
	TEST_ASSERT(search_for_checksum(fp2, sample_file, &checksum) == 0);
	TEST_ASSERT(strcmp(sample_sha1_str, checksum) == 0);

cleanup:
	e1 ? free_element(e1) : (void)0;
	e2 ? free_element(e2) : (void)0;
	fp1 ? fclose(fp1) : 0;
	fp2 ? fclose(fp2) : 0;
	cleanup_test_environment(path, files);
	free(checksum);
	remove(sample_file);
	remove(fp1str);
	remove(fp2str);
}

void test_create_removed_list(enum TEST_STATUS* status){
	FILE* fp1 = NULL;
	const char* fp1str = "checksum1.txt";
	FILE* fp2 = NULL;
	const char* fp2str = "checksum2.txt";
	char* tmp = NULL;

	const char* path = "TEST_ENVIRONMENT";
	char** files = NULL;
	size_t files_len = 0;

	size_t n_removed = 0;
	size_t ctr = 0;

	size_t i;

	setup_test_environment_basic(path, &files, &files_len);

	fp1 = fopen(fp1str, "wb");
	TEST_ASSERT(fp1);

	/* do the same shuffle as above */
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
	TEST_ASSERT_FREE(fp1, fclose);

	/* remove every other file */
	for (i = 1; i < files_len; i += 2){
		remove(files[i]);
		n_removed++;
	}

	/* create a list of removed files in fp2str based on fp1str */
	TEST_ASSERT(create_removed_list(fp1str, fp2str) == 0);

	fp2 = fopen(fp2str, "rb");
	TEST_ASSERT(fp2);

	while ((tmp = get_next_removed(fp2)) != NULL){
		printf("Removed: %s\n", tmp);
		/* count the amount of files in the list */
		ctr++;
		free(tmp);
	}
	/* number removed should equal number of files in list */
	TEST_ASSERT(n_removed == ctr);

cleanup:
	cleanup_test_environment(path, files);
	remove(fp1str);
	remove(fp2str);
}

int main(void){
	struct unit_test tests[] = {
		MAKE_TEST(test_bytes_to_hex),
		MAKE_TEST(test_checksum),
		MAKE_TEST(test_sort_checksum_file),
		MAKE_TEST(test_search_for_checksum),
		MAKE_TEST(test_create_removed_list)
	};

	log_setlevel(LEVEL_INFO);
	START_TESTS(tests);
	return 0;
}
