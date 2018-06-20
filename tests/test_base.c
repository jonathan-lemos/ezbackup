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
#include <setjmp.h>

enum PRINT_COLOR{
	COLOR_RED,
	COLOR_YELLOW,
	COLOR_GREEN,
	COLOR_BLUE,
	COLOR_NONE
};

static jmp_buf s_jumpbuffer;
static volatile sig_atomic_t s_last_signal;

static void vfprintf_color(enum PRINT_COLOR pc, FILE* stream, const char* format, va_list ap){
	if (!isatty(fileno(stream)) && errno == ENOTTY){
		pc = COLOR_NONE;
	}

	switch (pc){
	case COLOR_RED:
		fprintf(stream, "\033[31m");
		break;
	case COLOR_YELLOW:
		fprintf(stream, "\033[33m");
		break;
	case COLOR_GREEN:
		fprintf(stream, "\033[32m");
		break;
	case COLOR_BLUE:
		fprintf(stream, "\033[36m");
		break;
	default:
		break;
	}

	vfprintf(stream, format, ap);
	if (pc != COLOR_NONE){
		fprintf(stream, "\033[m");
	}
	fflush(stream);
}

static void internal_error_if_false(int condition, const char* file, int line, const char* msg){
	if (condition){
		return;
	}

	log_red("INTERNAL ERROR (%s:%d): %s\n", file, line, msg);
	abort();
}
#define INTERNAL_ERROR_IF_FALSE(condition) internal_error_if_false((intptr_t)(condition), __FILE__, __LINE__, #condition)

static void sig_longjmp(int signo){
	s_last_signal = signo;
	longjmp(s_jumpbuffer, signo);
}

static void handle_signal(void){
	switch (s_last_signal){
	case SIGABRT:
		log_red("SIGABRT sent to program. Exiting");
		exit(1);
		break;
	case SIGSEGV:
		log_red("Caught signal SIGSEGV");
		break;
	case SIGINT:
		log_yellow("SIGINT sent to program. Exiting");
		exit(0);
		break;
	case 0:
		break;
	default:
		log_blue("Caught signal %d", s_last_signal);
		break;
	}
	s_last_signal = 0;
}

/*
   static char* abs_path(const char* rel_path){
   static char rpath[4096];

   if (rel_path[0] == '/'){
   strcpy(rpath, rel_path);
   return rpath;
   }

   INTERNAL_ERROR_IF_FALSE(getcwd(rpath, sizeof(rpath)) != NULL);
   INTERNAL_ERROR_IF_FALSE(strlen(rpath) + 1 + strlen(rel_path) < sizeof(rpath) - 1);
   if (rpath[strlen(rpath - 1)] != '/'){
   strcat(rpath, "/");
   }
   strcat(rpath, rel_path);

   return rpath;
   }
   */

static void fprintf_color(enum PRINT_COLOR pc, FILE* stream, const char* format, ...){
	va_list ap;
	va_start(ap, format);
	vfprintf_color(pc, stream, format, ap);
	va_end(ap);
}

void log_red(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	vfprintf_color(COLOR_RED, stderr, format, ap);
	va_end(ap);
}

void log_yellow(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	vfprintf_color(COLOR_YELLOW, stderr, format, ap);
	va_end(ap);
}

void log_green(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	vfprintf_color(COLOR_GREEN, stderr, format, ap);
	va_end(ap);
}

void log_blue(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	vfprintf_color(COLOR_BLUE, stderr, format, ap);
	va_end(ap);
}

int test_assert(int condition, const char* file, int line, const char* msg){
	if (condition){
		return 0;
	}

	log_red("Assertion Failed (%s:%d): %s\n", file, line, msg);
	return 1;
}

void set_signal_handler(void){
	struct sigaction sa;
	sa.sa_handler = sig_longjmp;
	sigfillset(&(sa.sa_mask));
	sa.sa_flags = SA_RESTART;

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);

	if (setjmp(s_jumpbuffer)){
		handle_signal();
		log_yellow("A setjmp() location was not specified, so the program cannot continue");
		exit(1);
	}
}

void create_file(const char* name, const unsigned char* data, int len){
	FILE* fp;

	fp = fopen(name, "wb");
	INTERNAL_ERROR_IF_FALSE(fp);

	fwrite(data, 1, len, fp);
	INTERNAL_ERROR_IF_FALSE(ferror(fp) == 0);
	INTERNAL_ERROR_IF_FALSE(fclose(fp) == 0);
}

int memcmp_file_data(const char* file, const unsigned char* data, int data_len){
	FILE* fp;
	int i;

	fp = fopen(file, "rb");
	INTERNAL_ERROR_IF_FALSE(fp);

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
	INTERNAL_ERROR_IF_FALSE(fp1);
	fp2 = fopen(file2, "rb");
	INTERNAL_ERROR_IF_FALSE(fp2);

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
 * 			path/excl/exfile_noacc.txt (0000)
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
	INTERNAL_ERROR_IF_FALSE(getcwd(cwd, sizeof(cwd)) != NULL);
	INTERNAL_ERROR_IF_FALSE(mkdir(path, 0755) == 0);
	INTERNAL_ERROR_IF_FALSE(chdir(path) == 0);

	/* make dir1 */
	INTERNAL_ERROR_IF_FALSE(mkdir("dir1", 0755) == 0);
	INTERNAL_ERROR_IF_FALSE(chdir("dir1") == 0);
	/* make dir1's files */
	for (i = 0; i < sizeof(dir1_files) / sizeof(dir1_files[0]); ++i){
		unsigned char* data;
		size_t len = rand() % 1024;
		size_t j;

		/* fill with random data from 0-1023 bytes */
		data = malloc(len);
		INTERNAL_ERROR_IF_FALSE(data);

		for (j = 0; j < len; ++j){
			data[i] = rand() % 256;
		}

		/* create d1_file_01.txt */
		sprintf(dir1_files[i], "d1file_%02lu.txt", i);
		create_file(dir1_files[i], data, len);

		free(data);
	}
	/* go back to base dir */
	INTERNAL_ERROR_IF_FALSE(chdir("..") == 0);

	INTERNAL_ERROR_IF_FALSE(mkdir("dir2", 0755) == 0);
	INTERNAL_ERROR_IF_FALSE(chdir("dir2") == 0);
	for (i = 0; i < sizeof(dir2_files) / sizeof(dir2_files[0]); ++i){
		unsigned char* data;
		size_t len = rand() % 1024;
		size_t j;

		data = malloc(len);
		INTERNAL_ERROR_IF_FALSE(data);

		for (j = 0; j < len; ++j){
			data[i] = rand() % 256;
		}

		sprintf(dir2_files[i], "d2file_%02lu.txt", i);
		create_file(dir2_files[i], data, len);

		free(data);
	}
	INTERNAL_ERROR_IF_FALSE(chdir("..") == 0);

	INTERNAL_ERROR_IF_FALSE(mkdir("excl", 0755) == 0);
	INTERNAL_ERROR_IF_FALSE(chdir("excl") == 0);
	for (i = 0; i < sizeof(excl_files) / sizeof(excl_files[0]); ++i){
		unsigned char* data;
		size_t len = rand() % 1024;
		size_t j;

		data = malloc(len);
		INTERNAL_ERROR_IF_FALSE(data);

		for (j = 0; j < len; ++j){
			data[i] = rand() % 256;
		}

		sprintf(excl_files[i], "exfile_%02lu.txt", i);
		create_file(excl_files[i], data, len);

		free(data);
	}
	sprintf(excl_noacc, "exfile_noacc.txt");
	create_file(excl_noacc, (const unsigned char*)"noacc", strlen("noacc"));
	INTERNAL_ERROR_IF_FALSE(chmod(excl_noacc, 0000) == 0);

	INTERNAL_ERROR_IF_FALSE(chdir("..") == 0);

	INTERNAL_ERROR_IF_FALSE(mkdir("noaccess", 0000) == 0);

	INTERNAL_ERROR_IF_FALSE(chdir("..") == 0);
}

void cleanup_test_environment(const char* path){
	struct dirent* dnt;
	DIR* dp = opendir(path);

	INTERNAL_ERROR_IF_FALSE(dp);
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
			cleanup_test_environment(path_true);
			rmdir(path_true);
			free(path_true);
			continue;
		}

		remove(path_true);
		free(path_true);
	}
	closedir(dp);
	rmdir(path);
}

int run_tests(const struct unit_test* tests, size_t len){
	size_t i;
	int n_succeeded = 0;
	int n_failed = 0;
	for (i = 0; i < len; ++i){
		if (setjmp(s_jumpbuffer)){
			handle_signal();
			log_red("Test %lu of %lu (%s) crashed", i + 1, len + 1, tests[i].func_name);
			n_failed++;
		}
		else if (tests[i].func() != 0){
			log_red("Test %lu of %lu (%s) failed", i + 1, len + 1, tests[i].func_name);
			n_failed++;
		}
		else{
			log_green("Test %lu of %lu (%s) succeeded", i + 1, len + 1, tests[i].func_name);
			n_succeeded++;
		}

		printf("\n");
	}
	printf("\nResults: \n");
	log_green("%d of %lu succeeded.\n", n_succeeded, len);
	log_red("%d of %lu failed.\n", n_failed, len);
	return n_failed;
}
