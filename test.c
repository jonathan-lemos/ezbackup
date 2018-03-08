#define __TESTING__
#include "checksum.h"
#include "checksumsort.h"
#include "crypt.h"
#include "error.h"
#include "fileiterator.h"
#include "options.h"
#include "progressbar.h"
#include "readfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <assert.h>

static void my_pause(void){
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
	printf("Enter a character to continue...");
	getchar();
}

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
	const char* file1 = "file1.txt";
	const char* file2 = "file2.txt";
	const char* file3 = "file3.txt";
	const char* file4 = "file4.txt";
	element** elems = (element**)-1;
	char** out = (char**)-1;
	size_t n_files = -1;

	FILE* fp = (FILE*)-1;
	int i = -1;

	/* make temp files */
	fp = fopen(file1, "wb");
	assert(fp);
	fprintf(fp, "ayylmao00");
	fclose(fp);

	fp = fopen(file2, "wb");
	assert(fp);
	fprintf(fp, "ayylmao0");
	fclose(fp);

	fp = fopen(file3, "wb");
	assert(fp);
	fprintf(fp, "ayylmao3");
	fclose(fp);

	fp = fopen(file4, "wb");
	assert(fp);
	fprintf(fp, "ayylmao4");
	fclose(fp);

	/* open checksum for writing */
	fp = fopen(checksum_file, "wb");
	assert(fp);

	/* add files to checksum file */
	assert(add_checksum_to_file(file3, "sha1", fp) == 0);
	assert(add_checksum_to_file(file2, "sha1", fp) == 0);
	assert(add_checksum_to_file(file1, "sha1", fp) == 0);
	assert(add_checksum_to_file(file4, "sha1", fp) == 0);
	fclose(fp);

	/* reopen for reading */
	fopen(checksum_file, "rb");
	assert(fp);

	/* get the checksum elements */
	/* also assert they are in the correct order */
	elems = malloc(sizeof(*elems) * 4);
	assert(elems);
	elems[0] = get_next_checksum_element(fp);
	assert(strcmp(elems[0]->file, "file3.txt") == 0);
	elems[1] = get_next_checksum_element(fp);
	assert(strcmp(elems[1]->file, "file2.txt") == 0);
	elems[3] = get_checksum_element_index(fp, 3);
	assert(strcmp(elems[3]->file, "file4.txt") == 0);
	elems[2] = get_checksum_element_index(fp, 2);
	assert(strcmp(elems[2]->file, "file1.txt") == 0);

	/* close the file */
	for (i = 0; i < 4; ++i) free(elems[i]);
	fclose(fp);

	/* sorting the file */
	assert(create_initial_runs(checksum_file, &out, &n_files) == 0);
	assert(merge_files(out, n_files, sorted_file) == 0);

	/* checking that it's sorted */
	fp = fopen(sorted_file, "rb");
	assert(fp);
	elems[0] = get_next_checksum_element(fp);
	assert(strcmp(elems[0]->file, "file1.txt") == 0);
	elems[1] = get_next_checksum_element(fp);
	assert(strcmp(elems[1]->file, "file2.txt") == 0);
	elems[2] = get_next_checksum_element(fp);
	assert(strcmp(elems[2]->file, "file3.txt") == 0);
	elems[3] = get_next_checksum_element(fp);
	assert(strcmp(elems[3]->file, "file4.txt") == 0);

	/* closing file */
	for (i = 0; i < 4; ++i) free(elems[i]);
	fclose(fp);

	/* cleanup */
	for (i = 0; i < (int)n_files; ++i) free(out[i]);
	free(out);
	free(elems);

	remove(checksum_file);
	remove(sorted_file);
	remove(file1);
	remove(file2);
	remove(file3);
	remove(file4);
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
	int tmp;

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
	tmp = crypt_gen_keys((unsigned char*)"password", strlen("password"), NULL, 1, &fk);
	assert(tmp == 0);
	assert(crypt_decrypt(file_crypt, &fk, file_decrypt) == 0);

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

	tar_extract_file(tartemp, "/file", "test.txt", 1);

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
	const char* argv[] = {
		"-v",
		"-c",
		"gzip",
		"-C",
		"sha512",
		"-e",
		"camellia-256-cbc",
		"-o",
		"file.txt",
		"-x",
		"/exclude1",
		"/exclude2",
		"/in1",
		"/in2",
		"/in3"
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

	assert(write_options_tofile("/dev/stdout", &opt) == 0);
	my_pause();
	free_options(&opt);

	parse_options_menu(&opt);
	free_options(&opt);
}

void test_progressbar_h(void){
	progress* p;
	int i;

	p = start_progress("Testing progressbar.h", 10);
	assert (p);
	for (i = 0; i < 10; ++i){
		inc_progress(p, 1);
	}
	finish_progress(p);
}

int main(void){
	const char* file = "secret.txt";
	FILE* fp;

	fp = fopen(file, "w");
	assert(fp);
	fprintf(fp, "secret");
	fclose(fp);

	test_checksum_h();
	printf_green("Checksum.h test succeeded\n");
	test_crypt_h(file);
	printf_green("Crypt.h test succeeded\n");
	test_maketar_h(file);
	printf_green("Maketar.h test succeeded\n");
	remove(file);
	return 0;
	test_fileiterator_h("/home/jonathan/Documents\n");
	printf_green("Fileiterator.h test succeeded\n");
	test_progressbar_h();
	printf_green("Progressbar.h test succeeded\n");
	test_options_h();
	printf_green("Options.h test succeeded\n");

	remove(file);
	return 0;
}
