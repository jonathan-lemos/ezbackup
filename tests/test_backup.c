#include "test_base.h"
#include "../backup.h"
#include "../error.h"
#include <errno.h>
#include <sys/resource.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/stat.h>

const char* const sample_file1 = "file1.txt";
const char* const sample_data1 = "test";
const char* const sample_hash1 = "A94A8FE5CCB19BA61C4C0873D391E987982FBBD3";
const char* const sample_file2 = "file2.txt";
const char* const sample_data2 = "hello world";
const char* const sample_hash2 = "2AAE6C35C94FCFB415DBE95F408B9CE91EE846ED";
const char* const sample_checksum_file = "checksums.txt";
const char* const sample_checksum_data = \
"file1.txt\0""A94A8FE5CCB19BA61C4C0873D391E987982FBBD3\n"\
"file2.txt\0""2AAE6C35C94FCFB415DBE95F408B9CE91EE846ED\n";
const char* const sample_archive = "archive.tar.gz";
const char* const sample_archive_crypt = "archive.tar.gz.crypt";
const char* const sample_output_file = "checksums_extracted.txt";
const char* const sample_password = "hunter2";
const char* const sample_config_file = "config.conf";

void test_disable_core_dumps(void){
	struct rlimit rl;

	printf_blue("Testing disable/enable_core_dumps()\n");

	printf_yellow("Calling disable_core_dumps()\n");
	massert(disable_core_dumps() == 0);

	printf_yellow("Checking that it worked\n");
	massert(getrlimit(RLIMIT_CORE, &rl) == 0);
	massert(rl.rlim_cur == 0 && rl.rlim_max == 0);

	printf_yellow("Calling enable_core_dumps()\n");
	if (enable_core_dumps() != 0){
		printf_red("Not supported on this system\n");
	}

	printf_yellow("Checking that it worked\n");
	if (getrlimit(RLIMIT_CORE, &rl) != 0 ||
		(rl.rlim_cur > 0 && rl.rlim_max > 0)){
		printf_red("enable_core_dumps() failed\n");
	}

	printf_green("Finished testing disable/enable_core_dumps()\n");
}

void test_extract_prev_checksums(void){
	TAR* tarchive;
	char cmd_buf[256];

	printf_blue("Testing extract_prev_checksums()\n");

	create_file(sample_checksum_file, (const unsigned char*)sample_checksum_data, strlen(sample_checksum_data));

	printf_yellow("Creating tar\n");
	tarchive = tar_create(sample_archive, COMPRESSOR_GZIP, 0);
	massert(tarchive);
	massert(tar_add_file_ex(tarchive, sample_checksum_file, "/checksums", 0, NULL) == 0);
	massert(tar_close(tarchive) == 0);

	printf_yellow("Encrypting it\n");
	sprintf(cmd_buf, "openssl aes-256-cbc -e -salt -in %s -out %s -pass pass:%s", sample_archive, sample_archive_crypt, sample_password);
	printf("%s\n", cmd_buf);
	system(cmd_buf);

	printf_yellow("Calling extract_prev_checksums()\n");
	printf("Password: \"hunter2\"\n");
	massert(extract_prev_checksums(sample_archive_crypt, sample_output_file, EVP_aes_256_cbc(), 1) == 0);

	printf_yellow("Checking that the files match\n");
	massert(memcmp_file_file(sample_checksum_file, sample_output_file) == 0);

	remove(sample_checksum_file);
	remove(sample_archive);
	remove(sample_archive_crypt);
	remove(sample_output_file);

	printf_green("Finished testing extract_prev_checksums()\n\n");
}

void test_encrypt_file(void){
	char openssl_cmd[256];
	printf_blue("Testing encrypt_file()\n");

	create_file(sample_checksum_file, (const unsigned char*)sample_checksum_data, strlen(sample_checksum_data));

	printf_yellow("Calling encrypt_file()\n");
	massert(encrypt_file(sample_checksum_file, sample_file1, EVP_aes_256_cbc(), 1) == 0);

	printf_yellow("Calling openssl decryption\n");
	sprintf(openssl_cmd, "openssl aes-256-cbc -d -salt -in %s -out %s", sample_file1, sample_output_file);
	printf("%s\n", openssl_cmd);
	system(openssl_cmd);

	printf_yellow("Checking that the files match\n");
	massert(memcmp_file_file(sample_checksum_file, sample_output_file) == 0);

	remove(sample_checksum_file);
	remove(sample_file1);
	remove(sample_output_file);

	printf_green("Finished testing encrypt_file()\n\n");
}

void test_rename_ex(void){
	printf_blue("Testing rename_ex()\n");

	create_file(sample_file1, (const unsigned char*)sample_data1, strlen(sample_data1));

	printf_yellow("Calling rename_ex()\n");
	massert(rename_ex(sample_file1, sample_file2) == 0);

	create_file(sample_file1, (const unsigned char*)sample_data1, strlen(sample_data1));

	printf_yellow("Checking that it worked\n");
	massert(memcmp_file_file(sample_file1, sample_file2) == 0);

	remove(sample_file1);
	remove(sample_file2);

	printf_green("Finished testing rename_ex()\n\n");
}

static char* rel_path(const char* abs_path){
	static char rpath[4096];

	getcwd(rpath, sizeof(rpath));
	strcat(rpath, "/");
	strcat(rpath, abs_path);

	return rpath;
}

static void rmdir_recursive(const char* path){
	struct dirent* dnt;
	DIR* dp = opendir(path);

	massert(dp);
	while ((dnt = readdir(dp)) != NULL){
		char* path_true;
		struct stat st;

		if (!strcmp(dnt->d_name, ".") || !strcmp(dnt->d_name, "..")){
			continue;
		}

		path_true = malloc(strlen(path) + strlen(dnt->d_name) + 3);
		strcpy(path_true, path);
		if (path[strlen(path) - 1] != '/'){
			strcat(path_true, "/");
		}
		strcat(path_true, dnt->d_name);

		lstat(path_true, &st);
		if (S_ISDIR(st.st_mode)){
			rmdir_recursive(path_true);
			free(path_true);
			continue;
		}

		remove(path_true);
		free(path_true);
	}
	closedir(dp);
	rmdir(path);
}

void test_backup(void){
	struct options* opt;
	struct options* opt_prev;
	char files_dir1[10][64];
	char files_dir2[10][64];
	char files_ex1[10][64];
	int i;

	printf_blue("Testing backup()\n");

	printf_yellow("Building initial environment()\n");
	massert((opt = get_default_options() == NULL));

	opt.directories = malloc(sizeof(*(opt.directories)) * 3);
	massert(opt.directories);
	opt.directories_len = 3;

	opt.exclude = malloc(sizeof(*(opt.exclude)) * 1);
	massert(opt.exclude);
	opt.exclude_len = 1;

	/* making ~/dir1 */
	if (mkdir(rel_path("dir1"), 0755) != 0){
		log_msg(__FILE__, __LINE__, LEVEL_WARNING, "Failed to make ~/dir1 (%s)", strerror(errno));
	}
	opt.directories[0] = malloc(strlen(rel_path("dir1")) + 1);
	massert(opt.directories[0]);
	strcpy(opt.directories[0], rel_path("dir1"));

	/* making ~/dir2 */
	if (mkdir(rel_path("dir2"), 0755) != 0){
		log_msg(__FILE__, __LINE__, LEVEL_WARNING, "Failed to make ~/dir2 (%s)", strerror(errno));
	}
	opt.directories[1] = malloc(strlen(rel_path("dir2")) + 1);
	massert(opt.directories[1]);
	strcpy(opt.directories[1], rel_path("dir2"));

	/* making ~/ex1 (exclude directory) */
	if (mkdir(rel_path("ex1"), 0755) != 0){
		log_msg(__FILE__, __LINE__, LEVEL_WARNING, "Failed to make ~/ex1 (%s)", strerror(errno));
	}
	opt.exclude[0] = malloc(strlen(rel_path("ex1")) + 1);
	massert(opt.exclude[0]);
	strcpy(opt.exclude[0], rel_path("ex1"));

	/* making ~/noaccess (no permission to access) */
	if (mkdir(rel_path("noaccess"), 0755) != 0){
		log_msg(__FILE__, __LINE__, LEVEL_WARNING, "Failed to make ~/noaccess (%s)", strerror(errno));
	}
	opt.directories[2] = malloc(strlen(rel_path("noaccess")) + 1);
	massert(opt.directories[2]);
	strcpy(opt.directories[2], rel_path("noaccess"));

	for (i = 0; i < 10; ++i){
		sprintf(files_dir1[i], "dir1/d1_%03d", i);
		sprintf(files_dir2[i], "dir2/d2_%03d", i);
		sprintf(files_ex1[i], "ex1/ex_%03d", i);
	}
	for (i = 0; i < 10; ++i){
		char data[64];
		sprintf(data, "%s...%d", files_dir1[i], i);
		create_file(rel_path(files_dir1[i]), (const unsigned char*)data, strlen(data));

		sprintf(data, "%s...%d", files_dir2[i], i);
		create_file(rel_path(files_dir2[i]), (const unsigned char*)data, strlen(data));

		sprintf(data, "%s...%d", files_ex1[i], i);
		create_file(rel_path(files_ex1[i]), (const unsigned char*)data, strlen(data));
	}

	massert(backup(&opt, NULL) == 0);
	massert(write_config_file(&opt, sample_config_file) == 0);

	printf_yellow("Removing/adding select files\n");
	remove(rel_path(files_dir1[3]));
	strcpy(files_dir1[3], "dir1/d1_999");
	create_file(rel_path(files_dir1[3]), (const unsigned char*)"changed", sizeof("changed"));

	remove(rel_path(files_dir2[3]));
	strcpy(files_dir2[3], "dir2/d2_999");
	create_file(rel_path(files_dir2[3]), (const unsigned char*)"changed", sizeof("changed"));

	remove(rel_path(files_ex1[3]));
	strcpy(files_ex1[3], "ex1/ex_999");
	create_file(rel_path(files_ex1[3]), (const unsigned char*)"changed", sizeof("changed"));

	opt_prev = opt;
	printf_yellow("Testing backup() with previous backup\n");
	massert(backup(&opt, &opt_prev) == 0);

	printf_yellow("Cleaning up environment\n");
	remove(sample_config_file);

	rmdir_recursive(rel_path("dir1"));
	rmdir_recursive(rel_path("dir2"));
	rmdir_recursive(rel_path("ex1"));
	chmod(rel_path("noaccess"), 0755);
	rmdir(rel_path("noaccess"));

	printf_green("Finished testing backup()\n\n");
}

void test_get_default_backup_name(void){
	char* name;
	struct options opt;

	printf_blue("Testing get_default_backup_name()\n");
	massert(get_default_options(&opt) == 0);

	printf_yellow("Calling get_default_backup_name()\n");
	massert(get_default_backup_name(&opt, &name) == 0);
	printf("%s\n", name);

	free(name);

	printf_green("Finished testing get_default_backup_name()\n\n");
}

int main(void){
	set_signal_handler();
	log_setlevel(LEVEL_INFO);
	/*
	   test_disable_core_dumps();
	   test_extract_prev_checksums();
	   test_encrypt_file();
	   test_rename_ex();
	   */
	test_backup();
	test_get_default_backup_name();
	return 0;
}
