#include "test_base.h"
#include "../crypt.h"
#include "../checksum.h"
#include <stdlib.h>
#include <string.h>

static const char* const sample_file = "crypt.txt";
static unsigned char sample_data[999];
static const char* const sample_file_crypt = "crypt.txt.aes";
static const char* const sample_file_crypt2 = "crypt2.txt.aes";
static const char* const sample_file_decrypt = "decrypt.txt";
static const char* const sample_file_decrypt2 = "decrypt2.txt";
static const char* const password = "password";
static const unsigned char salt[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void test_crypt_secure_memcmp(void){
	const unsigned char mem1[]  = { 'A', 'B', 'C' };
	const unsigned char mem2[]  = { 'A', 'B', 'C' };
	const unsigned char fail[] = { 'A', 'B', 'c'};

	printf_blue("Testing crypt_secure_memcmp()\n");

	massert(crypt_secure_memcmp(mem1, mem2, sizeof(mem1)) == 0);
	massert(crypt_secure_memcmp(mem2, mem1, sizeof(mem1)) == 0);
	massert(crypt_secure_memcmp(mem1, fail, sizeof(fail)) != 0);

	printf_green("Testing crypt_secure_memcmp() succeeded\n\n");
}

void test_crypt_getpassword(void){
	char buf[1024];
	int res;
	int len;
	int i;

	printf_blue("Testing crypt_getpassword()\n");

	do{
		massert((res = crypt_getpassword("Enter temp pass", "Verify temp pass", buf, sizeof(buf))) >= 0);
	}while (res != 0);
	printf("You entered %s\n", buf);
	len = strlen(password);

	printf_yellow("Calling crypt_scrub()\n");
	massert(crypt_scrub(buf, len) == 0);
	printf("passwd buf: ");
	for (i = 0; i < len; ++i){
		printf("%c", buf[i]);
	}
	printf("\n");

	printf_green("Finished testing crypt_getpassword()\n\n");
}

void test_crypt_encrypt(void){
	struct crypt_keys fk;
	char openssl_cmd[256];
	unsigned char* crypt1_sha1;
	unsigned int crypt1_sha1_len;
	unsigned char* crypt2_sha1;
	unsigned int crypt2_sha1_len;

	printf_blue("Testing crypt_encrypt()\n");

	create_file(sample_file, sample_data, sizeof(sample_data));

	printf_yellow("Calling openssl's encryption\n");
	sprintf(openssl_cmd, "openssl aes-256-cbc -e -S 0000000000000000 -in %s -out %s -pass pass:%s", sample_file, sample_file_crypt, password);
	printf("%s\n", openssl_cmd);
	system(openssl_cmd);

	printf_yellow("Calling crypt_encrypt()\n");
	massert(crypt_set_encryption("aes-256-cbc", &fk) == 0);
	massert(crypt_set_salt(salt, &fk) == 0);
	massert(crypt_gen_keys((const unsigned char*)password, strlen(password), NULL, 1, &fk) == 0);
	massert(crypt_encrypt(sample_file, &fk, sample_file_crypt2) == 0);
	crypt_free(&fk);

	printf_yellow("Checking that the files match\n");
	massert(checksum(sample_file_crypt, "sha1", &crypt1_sha1, &crypt1_sha1_len) == 0);
	massert(checksum(sample_file_crypt2, "sha1", &crypt2_sha1, &crypt2_sha1_len) == 0);
	massert(crypt1_sha1_len == crypt2_sha1_len);
	massert(memcmp(crypt1_sha1, crypt2_sha1, crypt1_sha1_len) == 0);

	printf_yellow("Cleanup\n");
	free(crypt1_sha1);
	free(crypt2_sha1);
	remove(sample_file);
	remove(sample_file_crypt);
	remove(sample_file_crypt2);

	printf_green("Finished testing crypt_encrypt()\n\n");
}

void test_crypt_decrypt(void){
	struct crypt_keys fk;
	char openssl_cmd[256];
	unsigned char* crypt1_sha1;
	unsigned int crypt1_sha1_len;
	unsigned char* crypt2_sha1;
	unsigned int crypt2_sha1_len;

	printf_blue("Testing crypt_decrypt()\n");

	create_file(sample_file, sample_data, sizeof(sample_data));

	printf_yellow("Calling openssl's encryption\n");
	sprintf(openssl_cmd, "openssl aes-256-cbc -e -S 0000000000000000 -in %s -out %s -pass pass:%s", sample_file, sample_file_crypt, password);
	printf("%s\n", openssl_cmd);
	system(openssl_cmd);

	printf_yellow("Calling openssl's decryption\n");
	sprintf(openssl_cmd, "openssl aes-256-cbc -d -salt -in %s -out %s -pass pass:%s", sample_file_crypt, sample_file_decrypt, password);
	printf("%s\n", openssl_cmd);
	system(openssl_cmd);
	massert(checksum(sample_file_decrypt, "sha1", &crypt1_sha1, &crypt1_sha1_len) == 0);

	printf_yellow("Calling crypt_decrypt()\n");
	massert(crypt_set_encryption("aes-256-cbc", &fk) == 0);
	massert(crypt_extract_salt(sample_file_crypt, &fk) == 0);
	massert(crypt_gen_keys((const unsigned char*)password, strlen(password), NULL, 1, &fk) == 0);
	massert(crypt_decrypt(sample_file_crypt, &fk, sample_file_decrypt2) == 0);
	crypt_free(&fk);

	printf_yellow("Checking that the files match\n");
	massert(checksum(sample_file_decrypt2, "sha1", &crypt2_sha1, &crypt2_sha1_len) == 0);
	massert(crypt1_sha1_len == crypt2_sha1_len);
	massert(memcmp(crypt1_sha1, crypt2_sha1, crypt1_sha1_len) == 0);

	printf_yellow("Cleanup\n");
	free(crypt1_sha1);
	free(crypt2_sha1);
	remove(sample_file);
	remove(sample_file_crypt);
	remove(sample_file_decrypt);
	remove(sample_file_decrypt2);

	printf_green("Finished testing crypt_decrypt()\n\n");
}

int main(void){
	unsigned long i;

	set_signal_handler();

	for (i = 0; i < sizeof(sample_data); ++i){
		sample_data[i] = i % 10 + '0';
	}

	test_crypt_secure_memcmp();
	test_crypt_getpassword();
	test_crypt_encrypt();
	test_crypt_decrypt();
	return 0;
}
