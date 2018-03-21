/*
 * Testing module
 * Copyright (C) 2018 Jonathan Lemos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define __TESTING__
#include "checksum.h"
#include "checksumsort.h"
#include "crypt.h"
#include "error.h"
#include "fileiterator.h"
#include "options.h"
#include "progressbar.h"
#include "readfile.h"
#include "cloud/mega.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <assert.h>

/*
static void my_pause(void){
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
	printf("Enter a character to continue...");
	getchar();
}
*/

static void printf_green(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	printf("\033[32m");
	vprintf(format, ap);
	printf("\033[m");
	fflush(stdout);
	va_end(ap);
}

void test_checksum_h(void){
	const char* checksum_file = "test_checksums.txt";
	const char* sorted_file = "sorted.txt";
	char files[100][100];
	FILE* fptrs[100];
	FILE* fp;
	element** elems = (element**)-1;
	char** out = (char**)-1;
	size_t n_files = -1;
	int i;
	char* str_checksum;

	/* make files test{1..100}.txt */
	for (i = 0; i < 100; ++i){
		sprintf(files[i], "test%02d.txt", i);
	}
	/* shuffle the list */
	for (i = 0; i < 100; ++i){
		char buf[100];
		int j = rand() % 100;
		strcpy(buf, files[i]);
		strcpy(files[i], files[j]);
		strcpy(files[j], buf);
	}
	/* write something to each file */
	for (i = 0; i < 100; ++i){
		fptrs[i] = fopen(files[i], "wb");
		assert(fptrs[i]);
		fprintf(fptrs[i], "test%d", i << 4);
		fclose(fptrs[i]);
	}

	/* open checksum for writing */
	fp = fopen(checksum_file, "wb");
	assert(fp);

	/* add files to checksum file */
	for (i = 0; i < 100; ++i){
		assert(add_checksum_to_file(files[i], "sha1", fp, NULL) == 0);
	}
	fclose(fp);

	/* reopen for reading */
	fp = fopen(checksum_file, "rb");
	assert(fp);

	/* sorting the file */
	assert(create_initial_runs(checksum_file, &out, &n_files) == 0);
	assert(merge_files(out, n_files, sorted_file) == 0);

	/* checking that it's sorted */
	fp = fopen(sorted_file, "rb");
	assert(fp);
	elems = malloc(100 * sizeof(*elems));
	assert(elems);
	for (i = 0; i < 100; ++i){
		char str[100];
		sprintf(str, "test%02d.txt", i);
		elems[i] = get_next_checksum_element(fp);
		assert(elems[i]);
		assert(strcmp(str, elems[i]->file) == 0);
	}

	assert(search_for_checksum(sorted_file, "test99.txt", &str_checksum) == 0);
	assert(strcmp(str_checksum, "82ACFEE866E781644ABB3E638A4E033EAB89C813") == 0);
	assert(search_for_checksum(sorted_file, "nexist.txt", &str_checksum) == 1);

	/* closing file */
	for (i = 0; i < 100; ++i) free_element(elems[i]);
	fclose(fp);

	/* cleanup */
	for (i = 0; i < (int)n_files; ++i) free(out[i]);
	free(out);

	free(elems);

	remove(checksum_file);
	remove(sorted_file);
	for (i = 0; i < 100; ++i) remove(files[i]);
}

void test_crypt_h(const char* file){
	crypt_keys fk;
	char command[256];
	unsigned char salt[8] = "\0\0\0\0\0\0\0";
	const char* file_crypt = "secret2.aes";
	const char* file_decrypt = "file_reg.txt";
	unsigned char buf1[8192];
	unsigned char buf2[8192];
	int buf1_len;
	int buf2_len;
	FILE* fp1;
	FILE* fp2;

	assert(file);

	/* encrypt file */
	sprintf(command, "openssl aes-256-cbc -e -S 0000000000000000 -in %s -out secret.aes -pass pass:password", file);

	system(command);

	assert(crypt_set_encryption("aes-256-cbc", &fk) == 0);
	assert(crypt_set_salt((unsigned char*)"\0\0\0\0\0\0\0", &fk) == 0);
	assert(crypt_gen_keys((unsigned char*)"password", strlen("password"), NULL, 1, &fk) == 0);
	assert(crypt_encrypt(file, &fk, file_crypt) == 0);
	assert(crypt_free(&fk) == 0);

	/* check crypt files are the same */
	fp1 = fopen("secret.aes", "rb");
	fp2 = fopen(file_crypt, "rb");
	assert(fp1 && fp2);

	buf1_len = read_file(fp1, buf1, sizeof(buf1));
	buf2_len = read_file(fp2, buf2, sizeof(buf2));

	assert(buf1_len == buf2_len);
	assert(memcmp(buf1, buf2, buf1_len) == 0);

	fclose(fp1);
	fclose(fp2);
	/* decrypt file */
	assert(crypt_set_encryption("aes-256-cbc", &fk) == 0);
	assert(crypt_extract_salt(file_crypt, &fk) == 0);
	assert(memcmp(fk.salt, salt, sizeof(salt)) == 0);
	assert(crypt_gen_keys((unsigned char*)"password", strlen("password"), NULL, 1, &fk) == 0);
	assert(crypt_decrypt(file_crypt, &fk, file_decrypt) == 0);
	assert(crypt_free(&fk) == 0);

	/* check decrypted file matches original */
	fp1 = fopen(file_decrypt, "rb");
	fp2 = fopen(file, "rb");
	assert(fp1 && fp2);

	buf1_len = read_file(fp1, buf1, sizeof(buf1));
	buf2_len = read_file(fp2, buf2, sizeof(buf2));

	assert(buf1_len == buf2_len);
	assert(memcmp(buf1, buf2, buf1_len) == 0);

	fclose(fp1);
	fclose(fp2);
	/* cleanup */
	remove("secret.aes");
	remove(file_crypt);
	remove(file_decrypt);
}

int fun(const char* file, const char* dir, struct stat* st, void* params){
	(void)dir;
	(void)st;
	(void)params;

	printf("%s\n", file);
	return 1;
}

int error(const char* file, int __errno, void* params){
	(void)params;

	printf("%s: %s\n", file, strerror(__errno));
	return 1;
}

void test_fileiterator_h(char* basedir){
	assert(basedir);

	enum_files(basedir, fun, NULL, error, NULL);
}

void test_maketar_h(const char* file){
	unsigned char* hash1;
	unsigned char* hash2;
	unsigned len1;
	unsigned len2;

	const char* tartemp = "tartemp.tar.gz";
	TAR* tp = tar_create(tartemp, COMPRESSOR_GZIP);

	tar_add_file(tp, file);
	tar_add_file_ex(tp, file, "/file", 1, "Testing tar_add_file_ex()");
	tar_close(tp);

	tar_extract_file(tartemp, "/file", "test.txt");

	assert(checksum(file, "sha256", &hash1, &len1) == 0);
	assert(checksum("test.txt", "sha256", &hash2, &len2) == 0);

	assert(len1 == len2);
	assert(memcmp(hash1, hash2, len1) == 0);

	free(hash1);
	free(hash2);

	remove(tartemp);
	remove("test.txt");
}

void test_options_h(void){
	options opt;
	char* argv[] = {
		"-v",
		"-c",
		"gzip",
		"-C",
		"sha512",
		"-e",
		"camellia-256-cbc",
		"-o",
		"file.txt",
		"/in1",
		"/in2",
		"/in3",
		"-x",
		"/exclude1",
		"/exclude2",
		"/exclude3"
	};
	int argc = sizeof(argv) / sizeof(argv[0]);

	usage("progname");
	parse_options_cmdline(argc, argv, &opt);

	assert(opt.flags == 1);
	assert(strcmp(opt.file_out, "file.txt") == 0);
	assert(opt.comp_algorithm == COMPRESSOR_GZIP);
	assert(strcmp(opt.hash_algorithm, "sha512") == 0);
	assert(strcmp(opt.enc_algorithm, "camellia-256-cbc") == 0);
	assert(strcmp(opt.exclude[0], "/exclude1") == 0);
	assert(strcmp(opt.exclude[1], "/exclude2") == 0);
	assert(strcmp(opt.directories[0], "/in1") == 0);
	assert(strcmp(opt.directories[1], "/in2") == 0);
	assert(strcmp(opt.directories[2], "/in3") == 0);
	free_options(&opt);

	parse_options_menu(&opt);
	assert(write_options_tofile("/dev/stdout", &opt) == 0);

	free_options(&opt);
}

void test_progressbar_h(void){
	progress* p;
	int i;

	p = start_progress("Testing progressbar.h", 10000);
	assert (p);
	for (i = 0; i < 10000; ++i){
		inc_progress(p, 1);
		usleep(666);
	}
	finish_progress(p);
}

void test_mega_cpp(void){
	MEGAhandle mh;

	assert(MEGAlogin("***REMOVED***", "***REMOVED***", &mh) == 0);
	puts_debug("Login successful");
	assert(MEGAupload("data.img", "/", "Uploading file", mh) == 0);
	assert(MEGAdownload("data.img", "download", "Downloading file", mh) == 0);
	puts_debug("Upload successful");
	MEGAlogout(mh);
}

int main(void){
	const char* file = "secret.txt";
	FILE* fp;

	log_setlevel(LEVEL_DEBUG);

	fp = fopen(file, "w");
	assert(fp);
	fprintf(fp, "secret");
	fclose(fp);

	test_progressbar_h();
/*	test_mega_cpp(); */
	return 0;

	test_checksum_h();
	printf_green("Checksum.h test succeeded\n");
	test_crypt_h(file);
	printf_green("Crypt.h test succeeded\n");
	test_maketar_h(file);
	printf_green("Maketar.h test succeeded\n");
	remove(file);
	test_fileiterator_h("/home/jonathan/Documents");
	printf_green("Fileiterator.h test succeeded\n");
	/*
	   test_progressbar_h();
	   printf_green("Progressbar.h test succeeded\n");
	   */
	test_options_h();
	printf_green("Options.h test succeeded\n");

	remove(file);
	return 0;
}
