/* test_base.c -- helper functions for tests
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "test_base.h"
#include "../log.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

void printf_red(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	printf("\033[31m");
	vprintf(format, ap);
	printf("\033[m");
	fflush(stdout);
	va_end(ap);
}

void printf_yellow(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	printf("\033[33m");
	vprintf(format, ap);
	printf("\033[m");
	fflush(stdout);
	va_end(ap);
}

void printf_green(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	printf("\033[32m");
	vprintf(format, ap);
	printf("\033[m");
	fflush(stdout);
	va_end(ap);
}

void printf_blue(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	printf("\033[36m");
	vprintf(format, ap);
	printf("\033[m");
	fflush(stdout);
	va_end(ap);
}

void __massert(int condition, const char* file, int line, const char* msg){
	if (condition){
		return;
	}

	printf("Assertion Failed (%s:%d): ", file, line);
	printf("%s\n", msg);
	abort();
}

static void handle_signals(int signo){
	switch(signo){
	case SIGABRT:
		printf_red("Caught signal SIGABRT\n");
		break;
	case SIGSEGV:
		printf_red("Caught signal SIGSEGV\n");
		break;
	case SIGINT:
		printf_yellow("Caught signal SIGINT\n");
		break;
	default:
		printf_blue("Caught signal %d\n", signo);
	}
	/* todo: somehow implement backtrace */
	exit(1);
}

void set_signal_handler(void){
	struct sigaction sa;
	sa.sa_handler = handle_signals;
	sigfillset(&(sa.sa_mask));
	sa.sa_flags = SA_RESTART;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
}

void create_file(const char* name, const unsigned char* data, int len){
	FILE* fp;

	fp = fopen(name, "wb");
	if (!fp){
		log_efopen(name);
		massert(0);
	}
	fwrite(data, 1, len, fp);
	massert(ferror(fp) == 0);
	massert(fclose(fp) == 0);
}

int memcmp_file_data(const char* file, const unsigned char* data, int data_len){
	FILE* fp;
	int i;

	fp = fopen(file, "rb");
	massert(fp);
	for (i = 0; i < data_len; ++i){
		int c;

		c = fgetc(fp);
		if (c != data[i]){
			fclose(fp);
			return c - data[i];
		}
	}
	if (fgetc(fp) != EOF){
		fclose(fp);
		return 1;
	}

	fclose(fp);
	return 0;
}

int memcmp_file_file(const char* file1, const char* file2){
	FILE* fp1;
	FILE* fp2;
	int c1, c2;

	fp1 = fopen(file1, "rb");
	massert(fp1);
	fp2 = fopen(file2, "rb");
	massert(fp2);

	do{
		c1 = fgetc(fp1);
		c2 = fgetc(fp2);
	}while (c1 == c2 && c1 != EOF);

	fclose(fp1);
	fclose(fp2);

	return c1 - c2;
}

int does_file_exist(const char* file){
	struct stat st;
	return stat(file, &st) == 0;
}

static char* abs_path(const char* rel_path){
	static char rpath[4096];

	massert(getcwd(rpath, sizeof(rpath)) != NULL);
	massert(strlen(rpath) + 1 + strlen(rel_path) < 4096 - 1);
	strcat(rpath, "/");
	strcat(rpath, rel_path);

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

		if (chmod(path_true, 0777) != 0){
			fprintf(stderr, "Failed to chmod %s (%s)\n", path_true, strerror(errno));
			free(path_true);
			continue;
		}

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

/*
 * Creates a test environment with the following structure.
 *
 * path (0755)
 * 		path/dir1 (0755)
 * 			path/dir1/d1file_{00-11}.txt (0666)
 * 		path/dir2 (0755)
 * 			path/dir2/d2file_{00-10}.txt (0666)
 * 		path/excl (0755)
 * 			path/excl/exfile_{00-09}.txt (0666)
 * 		path/noaccess (0000)
 */
void setup_test_environment(const char* path){
	char cwd[4096];
	char dir1_files[12][64];
	char dir2_files[11][64];
	char excl_files[10][64];
	char excl_noacc[64];
	size_t i;

	srand(0);

	/* make directory at path */
	massert(getcwd(cwd, sizeof(cwd)) != NULL);
	massert(mkdir(abs_path(path), 0755) == 0);
	massert(chdir(abs_path(path)) == 0);

	/* make dir1 */
	massert(mkdir("dir1", 0755) == 0);
	massert(chdir("dir1") == 0);
	/* make dir1's files */
	for (i = 0; i < sizeof(dir1_files) / sizeof(dir1_files[0]); ++i){
		unsigned char* data;
		size_t len = rand() % 1024;
		size_t j;

		/* fill with random data from 0-1023 bytes */
		data = malloc(len);
		massert(data);

		for (j = 0; j < len; ++j){
			data[i] = rand() % 256;
		}

		/* create d1_file_01.txt */
		sprintf(dir1_files[i], "d1file_%02lu.txt", i);
		create_file(dir1_files[i], data, len);

		free(data);
	}
	/* go back to base dir */
	massert(chdir("..") == 0);

	massert(mkdir("dir2", 0755) == 0);
	massert(chdir("dir2") == 0);
	for (i = 0; i < sizeof(dir2_files) / sizeof(dir2_files[0]); ++i){
		unsigned char* data;
		size_t len = rand() % 1024;
		size_t j;

		data = malloc(len);
		massert(data);

		for (j = 0; j < len; ++j){
			data[i] = rand() % 256;
		}

		sprintf(dir2_files[i], "d2file_%02lu.txt", i);
		create_file(dir2_files[i], data, len);

		free(data);
	}
	massert(chdir("..") == 0);

	massert(mkdir("excl", 0755) == 0);
	massert(chdir("excl") == 0);
	for (i = 0; i < sizeof(excl_files) / sizeof(excl_files[0]); ++i){
		unsigned char* data;
		size_t len = rand() % 1024;
		size_t j;

		data = malloc(len);
		massert(data);

		for (j = 0; j < len; ++j){
			data[i] = rand() % 256;
		}

		sprintf(excl_files[i], "exfile_%02lu.txt", i);
		create_file(excl_files[i], data, len);

		free(data);
	}
	sprintf(excl_noacc, "exfile_noacc.txt");
	create_file(excl_noacc, (const unsigned char*)"noacc", strlen("noacc"));
	massert(chmod(excl_noacc, 0000) == 0);

	massert(chdir("..") == 0);

	massert(mkdir("noaccess", 0000) == 0);
}

void cleanup_test_environment(const char* path){
	rmdir_recursive(path);
}
