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
void test_bytes_to_hex(void){
	char* out;

	printf_blue("Testing bytes_to_hex()\n");

	printf_yellow("Calling bytes_to_hex()\n");
	massert(bytes_to_hex(sample_sha1, sizeof(sample_sha1), &out) == 0);
	printf_yellow("Checking the output value\n");
	massert(strcmp(out, sample_sha1_str) == 0);

	free(out);
	printf_green("Finished testing bytes_to_hex()\n\n");
}

/* checksum() */
void test_checksum(void){
	unsigned char* out;
	unsigned len;

	printf_blue("Testing checksum()\n");

	create_file(sample_file, sample_data, sizeof(sample_data));

	/* hash that file and check it against the expected value */
	printf_yellow("Calling checksum()\n");
	massert(checksum(sample_file, EVP_sha1(), &out, &len) == 0);
	printf_yellow("Checking the output value\n");
	massert(memcmp(sample_sha1, out, len) == 0);

	free(out);
	remove(sample_file);
	printf_green("Finished testing checksum()\n\n");
}

/* add_checksum_to_file()
 * sort_checksum_file()
 * add_checksum_to_file() with previous file */
void test_sort_checksum_file(void){
	char files[100][64];
	char data[100][64];
	FILE* fp1;
	const char* fp1str = "checksum1.txt";
	FILE* fp2;
	const char* fp2str = "checksum2.txt";
	struct element* e1 = NULL;
	struct element* e2 = NULL;
	int i;

	printf_blue("Testing sort_checksum_file()\n");

	/* predictable shuffle */
	srand(0);

	printf_yellow("Making files\n");
	/* make output file */
	fp1 = fopen(fp1str, "wb");
	massert(fp1);
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
	printf_yellow("Calling add_checksum_to_file() (initial run)\n");
	for (i = 0; i < 100; ++i){
		int res;
		res = add_checksum_to_file(files[i], EVP_sha1(), fp1, NULL);
		massert(res >= 0);
		if (res == 1){
			printf("Old element: %s\n", files[i]);
		}
	}
	massert(fclose(fp1) == 0);
	/* sorting file */
	printf_yellow("Calling sort_checksum_file()\n");
	massert(sort_checksum_file(fp1str, fp2str) == 0);
	/* checking that it's sorted */
	fp2 = fopen(fp2str, "rb");
	massert(fp2);
	e1 = get_next_checksum_element(fp2);
	for (i = 0; i < 100 - 1; ++i){
		e2 = get_next_checksum_element(fp2);
		massert(strcmp(e1->file, e2->file) <= 0);
		free_element(e1);
		e1 = e2;
	}
	free_element(e2);
	/* moving sorted list to fp1 */
	massert(fclose(fp2) == 0);
	remove(fp1str);
	rename(fp2str, fp1str);
	fp1 = fopen(fp1str, "rb");
	massert(fp1);
	fp2 = fopen(fp2str, "wb");
	massert(fp2);

	printf_yellow("Changing data in select files\n");
	/* changing data */
	for (i = 1; i < 100; i += 3){
		create_file(files[i], (unsigned char*)data[i - 1], strlen(data[i - 1]));
	}
	/* checksum run with previous file */
	printf_yellow("Calling add_checksum_to_file() (2nd run)\n");
	for (i = 0; i < 100; ++i){
		int res;
		res = add_checksum_to_file(files[i], EVP_sha1(), fp2, fp1);
		massert(res >= 0);
		if (res == 1){
			printf("Old element: %s\n", files[i]);
		}
	}
	massert(fclose(fp1) == 0);
	massert(fclose(fp2) == 0);

	/* cleaning up */
	printf_yellow("Cleanup\n");
	for (i = 0; i < 100; ++i){
		remove(files[i]);
	}
	remove(fp1str);
	remove(fp2str);
	printf_green("Finished testing sort_checksum_file()\n\n");
}

void test_search_for_checksum(void){
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

	printf_blue("Testing sort_checksum_file()\n");

	/* predictable shuffle */
	srand(0);

	printf_yellow("Making files\n");
	/* make output file */
	fp1 = fopen(fp1str, "wb");
	massert(fp1);
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
	printf_yellow("Calling add_checksum_to_file() (initial run)\n");
	for (i = 0; i < 100; ++i){
		int res;
		res = add_checksum_to_file(files[i], EVP_sha1(), fp1, NULL);
		massert(res >= 0);
		if (res == 1){
			printf("Old element: %s\n", files[i]);
		}
	}
	create_file(sample_file, sample_data, sizeof(sample_data));
	add_checksum_to_file(sample_file, EVP_sha1(), fp1, NULL);

	massert(fclose(fp1) == 0);
	/* sorting file */
	printf_yellow("Calling sort_checksum_file()\n");
	massert(sort_checksum_file(fp1str, fp2str) == 0);
	massert(fclose(fp1) == 0);
	/* checking that it's sorted */
	fp2 = fopen(fp2str, "rb");
	massert(fp2);
	e1 = get_next_checksum_element(fp2);
	for (i = 0; i < 100 - 1; ++i){
		e2 = get_next_checksum_element(fp2);
		massert(strcmp(e1->file, e2->file) <= 0);
		free_element(e1);
		e1 = e2;
	}
	free_element(e2);

	printf_yellow("Calling search_for_checksum()");
	massert(search_for_checksum(fp2, "noexist", &checksum) > 0);
	massert(search_for_checksum(fp2, sample_file, &checksum) == 0);
	massert(strcmp(sample_sha1_str, checksum) == 0);

	/* cleaning up */
	printf_yellow("Cleanup\n");
	free(checksum);
	remove(sample_file);
	for (i = 0; i < 100; ++i){
		remove(files[i]);
	}
	remove(fp1str);
	remove(fp2str);
	printf_green("Finished testing search_for_checksum()\n\n");
}

void test_create_removed_list(void){
	char files[100][64];
	char data[100][64];
	FILE* fp1;
	const char* fp1str = "checksum1.txt";
	FILE* fp2;
	const char* fp2str = "checksum2.txt";
	char* tmp;
	int ctr = 0;

	int i;

	printf_blue("Testing create_removed_list()\n");

	printf_yellow("Making files\n");
	/* make output file */
	fp1 = fopen(fp1str, "wb");
	massert(fp1);
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
	printf_yellow("Calling add_checksum_to_file() (initial run)\n");
	for (i = 0; i < 100; ++i){
		int res;
		res = add_checksum_to_file(files[i], EVP_sha1(), fp1, NULL);
		massert(res >= 0);
		if (res == 1){
			printf("Old element: %s\n", files[i]);
		}
	}

	massert(fclose(fp1) == 0);
	fp1 = fopen(fp1str, "rb");
	massert(fp1);
	fp2 = fopen(fp2str, "wb");
	massert(fp2);

	for (i = 0; i < 100; i += 9){
		remove(files[i]);
	}
	printf_yellow("Calling create_removed_list()\n");
	massert(fclose(fp1) == 0);
	massert(fclose(fp2) == 0);
	massert(create_removed_list(fp1str, fp2str) == 0);
	fp2 = fopen(fp2str, "rb");
	massert(fp2);

	while ((tmp = get_next_removed(fp2)) != NULL){
		int buf;
		massert(sscanf(tmp, "file%03d.txt", &buf) == 1);
		massert(buf % 9 == 0);
		ctr++;
		free(tmp);
	}
	massert(ctr == 12);

	printf_yellow("Cleanup\n");
	remove(fp1str);
	remove(fp2str);
	for (i = 0; i < 100; ++i){
		remove(files[i]);
	}
	printf_green("Finished testing create_removed_list()\n\n");
}

void test_get_next_removed(void){
	printf_blue("Testing get_next_removed()\n");
	test_create_removed_list();
	printf_green("Finished testing get_next_removed()\n\n");
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
