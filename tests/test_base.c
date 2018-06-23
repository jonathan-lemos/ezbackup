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

#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

enum PRINT_COLOR{
	COLOR_RED,
	COLOR_YELLOW,
	COLOR_GREEN,
	COLOR_BLUE,
	COLOR_NONE
};

/* these two are needed to handle segfaults and other signals */
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

/* this is our true signal handler
 *
 * it's rather limited since signal handlers can only use
 * re-entrant functions
 *
 * the actual handling of the signal happens in handle_signal() */
static void sig_longjmp(int signo){
	s_last_signal = signo;
	/* longjmp to our previous setjmp() location */
	longjmp(s_jumpbuffer, signo);
}

static void handle_signal(void){
	switch (s_last_signal){
	/* SIGABRT == serious error
	 * no use trying to recover here
	 *
	 * furthermore, not exiting after SIGABRT causes assert() to not exit the program */
	case SIGABRT:
		log_red("SIGABRT sent to program. Exiting");
		exit(1);
		break;
	/* catches segfaults
	 *
	 * hopefully the heap is still in good condition when we do this
	 * otherwise the program will crash very soon after we handle this signal */
	case SIGSEGV:
		log_red("Caught signal SIGSEGV");
		break;
	/* catches ctrl+c
	 *
	 * ctrl+c means the user wants to exit, so we let them */
	case SIGINT:
		log_yellow("SIGINT sent to program. Exiting");
		exit(0);
		break;
	/* no signal */
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

void log_default(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	vfprintf_color(COLOR_NONE, stderr, format, ap);
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
	/* block all signals while signal handler is running */
	sigfillset(&(sa.sa_mask));
	/* re-enter functions where we left off */
	sa.sa_flags = SA_RESTART;

	/* i'll catch other signals if I come across them */
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);

	/* need a default landing point for signal handler's longjmp */
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

/* memcmp's the contents of a file to data */
int memcmp_file_data(const char* file, const unsigned char* data, int data_len){
	FILE* fp;
	int i;

	fp = fopen(file, "rb");
	INTERNAL_ERROR_IF_FALSE(fp);

	for (i = 0; i < data_len; ++i){
		int c;

		c = fgetc(fp);
		/* EOF = -1, lowest possible char */
		if (c != data[i]){
			fclose(fp);
			return c - data[i];
		}
	}
	/* if data_len is less than file len */
	if (fgetc(fp) != EOF){
		fclose(fp);
		return 1;
	}

	fclose(fp);
	return 0;
}

/* memcmp's the contents of two files */
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
	/* don't need to check c2 != EOF since c1 == c2 will catch it */
	}while (c1 == c2 && c1 != EOF);

	fclose(fp1);
	fclose(fp2);

	/* if c1 is EOF, this returns negative like it should
	 * if c2 is EOF, this returns positive like it should */
	return c1 - c2;
}

int does_file_exist(const char* file){
	struct stat st;
	return stat(file, &st) == 0;
}

/* strdup() is not guaranteed by the c standard */
static char* str_duplicate(const char* in){
	char* ret = malloc(strlen(in) + 1);
	INTERNAL_ERROR_IF_FALSE(ret);
	strcpy(ret, in);
	return ret;
}

/* make_path(3, "dir1", "dir2", "file.txt") -> "dir1/dir2/file.txt" */
static char* make_path(int n_components, ...){
	char* ret = NULL;
	va_list ap;
	int i;

	/* make an empty string so strcat() works properly */
	ret = calloc(1, 1);
	INTERNAL_ERROR_IF_FALSE(ret);

	va_start(ap, n_components);

	for (i = 0; i < n_components; ++i){
		const char* arg = va_arg(ap, const char*);

		/* if the string does not end with '/' and
		 * the next string does not begin with '/' */
		if (ret[strlen(ret) - 1] != '/' && arg[0] != '/'){
			/* add a '/' to the end. strlen() + 2 leaves room for '/' and '\0' */
			ret = realloc(ret, strlen(ret) + 2);
			INTERNAL_ERROR_IF_FALSE(ret);

			strcat(ret, "/");
		}

		/* strlen() + 1 leaves room for '\0' */
		ret = realloc(ret, strlen(ret) + strlen(arg) + 1);
		INTERNAL_ERROR_IF_FALSE(ret);

		strcat(ret, arg);
	}

	va_end(ap);
	return ret;
}

/*
 * Creates a test environment with the following structure.
 *
 * path (0755)
 *     path/file_{00-20}.txt (0666)
 */
void setup_test_environment_basic(const char* path, char*** out, size_t* out_len){
	char* files[20];
	size_t i;

	srand(0);

	INTERNAL_ERROR_IF_FALSE(mkdir(path, 0755) == 0);

	for (i = 0; i < ARRAY_LEN(files); ++i){
		unsigned char* data;
		char filename[64];
		size_t len = rand() % 1024;
		size_t j;

		/* fill with random data from 0-1023 bytes */
		data = malloc(len);
		INTERNAL_ERROR_IF_FALSE(data);

		for (j = 0; j < len; ++j){
			data[i] = rand() % ('Z' - 'A') + 'A';
		}

		/* create d1_file_XX.txt */
		sprintf(filename, "file_%02lu.txt", i);
		files[i] = make_path(2, path, filename);
		create_file(files[i], data, len);

		free(data);
	}

	if (!out){
		for (i = 0; i < ARRAY_LEN(files); ++i){
			free(files[i]);
		}
		return;
	}

	/* out is not NULL, so fill out with filenames */

	/* ARRAY_LEN + 1 makes sure last entry is NULL
	 * this is needed to iterate through the list without files_len */
	*out = calloc(ARRAY_LEN(files) + 1, sizeof(**out));
	INTERNAL_ERROR_IF_FALSE(*out);

	/* copy the filenames over and free the local ones */
	for (i = 0; i < ARRAY_LEN(files); ++i){
		(*out)[i] = str_duplicate(files[i]);
		free(files[i]);
	}

	if (out_len){
		*out_len = ARRAY_LEN(files);
	}
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
void setup_test_environment_full(const char* path, char*** out, size_t* out_len){
	char* dir1_files[12];
	char* dir2_files[11];
	char* excl_files[10];
	char* excl_noacc;
	const size_t total_len = ARRAY_LEN(dir1_files) + ARRAY_LEN(dir2_files) + ARRAY_LEN(excl_files) + 1;
	char* tmp;

	size_t i;
	size_t out_ptr = 0;

	srand(0);

	INTERNAL_ERROR_IF_FALSE(mkdir(path, 0755) == 0);

	/* make dir1 */
	tmp = make_path(2, path, "dir1");
	INTERNAL_ERROR_IF_FALSE(mkdir(tmp, 0755) == 0);
	free(tmp);

	/* make dir1's files */
	for (i = 0; i < ARRAY_LEN(dir1_files); ++i){
		unsigned char* data;
		char filename[64];
		size_t len = rand() % 1024;
		size_t j;

		/* fill with random data from 0-1023 bytes */
		data = malloc(len);
		INTERNAL_ERROR_IF_FALSE(data);

		for (j = 0; j < len; ++j){
			data[i] = rand() % ('Z' - 'A') + 'A';
		}

		/* create d1_file_01.txt */
		sprintf(filename, "d1file_%02lu.txt", i);
		dir1_files[i] = make_path(3, path, "dir1", filename);
		create_file(dir1_files[i], data, len);

		free(data);
	}

	/* make dir2 */
	tmp = make_path(2, path, "dir2");
	INTERNAL_ERROR_IF_FALSE(mkdir(tmp, 0755) == 0);
	free(tmp);

	for (i = 0; i < ARRAY_LEN(dir2_files); ++i){
		unsigned char* data;
		char filename[64];
		size_t len = rand() % 1024;
		size_t j;

		data = malloc(len);
		INTERNAL_ERROR_IF_FALSE(data);

		for (j = 0; j < len; ++j){
			data[i] = rand() % ('Z' - 'A') + 'A';
		}

		sprintf(filename, "d2file_%02lu.txt", i);
		dir2_files[i] = make_path(3, path, "dir2", filename);
		create_file(dir2_files[i], data, len);

		free(data);
	}

	/* make excl */
	tmp = make_path(2, path, "excl");
	INTERNAL_ERROR_IF_FALSE(mkdir(path, 0755) == 0);
	free(tmp);

	for (i = 0; i < sizeof(excl_files) / sizeof(excl_files[0]); ++i){
		unsigned char* data;
		char filename[64];
		size_t len = rand() % 1024;
		size_t j;

		data = malloc(len);
		INTERNAL_ERROR_IF_FALSE(data);

		for (j = 0; j < len; ++j){
			data[i] = rand() % ('Z' - 'A') + 'A';
		}

		sprintf(filename, "exfile_%02lu.txt", i);
		excl_files[i] = make_path(3, path, "excl", filename);
		create_file(excl_files[i], data, len);

		free(data);
	}
	excl_noacc = make_path(3, path, "excl", "exfile_noacc.txt");
	create_file(excl_noacc, (const unsigned char*)"noacc", strlen("noacc"));
	INTERNAL_ERROR_IF_FALSE(chmod(excl_noacc, 0000) == 0);

	INTERNAL_ERROR_IF_FALSE(mkdir("noaccess", 0000) == 0);

	if (!out){
		for (i = 0; i < ARRAY_LEN(dir1_files); ++i){
			free(dir1_files[i]);
		}
		for (i = 0; i < ARRAY_LEN(dir2_files); ++i){
			free(dir2_files[i]);
		}
		for (i = 0; i < ARRAY_LEN(excl_files); ++i){
			free(excl_files[i]);
		}
		free(excl_noacc);
		return;
	}

	/* total_len + 1 makes last entry NULL */
	*out = calloc(total_len + 1, sizeof(**out));
	INTERNAL_ERROR_IF_FALSE(*out);
	out_ptr = 0;

	for (i = 0; i < ARRAY_LEN(dir1_files[i]); ++i){
		(*out)[out_ptr] = str_duplicate(dir1_files[i]);
		free(dir1_files[i]);
		out_ptr++;
	}

	for (i = 0; i < ARRAY_LEN(dir2_files); ++i){
		(*out)[out_ptr] = str_duplicate(dir2_files[i]);
		free(dir2_files[i]);
		out_ptr++;
	}

	for (i = 0; i < ARRAY_LEN(excl_files); ++i){
		(*out)[out_ptr] = str_duplicate(excl_files[i]);
		free(excl_files[i]);
		out_ptr++;
	}

	(*out)[out_ptr] = str_duplicate(excl_noacc);

	if (out_len){
		*out_len = total_len;
	}
}

/* recursively removes files and directories at path
 * if files != NULL, frees files */
void cleanup_test_environment(const char* path, char** files){
	struct dirent* dnt;
	DIR* dp = opendir(path);

	if (files){
		size_t i;
		/* last entry guaranteed to be NULL */
		for (i = 0; files[i] != NULL; ++i){
			free(files[i]);
		}
		free(files);
	}

	INTERNAL_ERROR_IF_FALSE(dp);
	/* while there are entries in the current directory */
	while ((dnt = readdir(dp)) != NULL){
		char* path_true;
		struct stat st;

		/* "." and ".." are not real folders */
		if (!strcmp(dnt->d_name, ".") || !strcmp(dnt->d_name, "..")){
			continue;
		}

		path_true = make_path(2, path, dnt->d_name);

		/* some of the files are 0000
		 * they need to be at least 0700 so they can be removed */
		if (chmod(path_true, 0700) != 0){
			fprintf(stderr, "Failed to chmod %s (%s)\n", path_true, strerror(errno));
			free(path_true);
			continue;
		}

		/* lstat does not follow symlinks */
		lstat(path_true, &st);
		/* if the current file is a directory */
		if (S_ISDIR(st.st_mode)){
			/* recursively remove files in that directory */
			cleanup_test_environment(path_true, NULL);
			free(path_true);
			continue;
		}

		/* otherwise remove the file */
		remove(path_true);
		free(path_true);
	}
	closedir(dp);
	/* finally, remove the base directory */
	rmdir(path);
}

int run_tests(const struct unit_test* tests, size_t len){
	size_t i;
	int n_succeeded = 0;
	int n_failed = 0;
	for (i = 0; i < len; ++i){
		/* if a test sends a signal, execution will jump here */
		if (setjmp(s_jumpbuffer)){
			/* take appropriate action based on the signal */
			handle_signal();
			log_red("Test %lu of %lu (%s) crashed", i + 1, len + 1, tests[i].func_name);
			n_failed++;
		}
		/* execute the test and see if it returns TEST_SUCCESS or not */
		else if (tests[i].func() != TEST_SUCCESS){
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
	/* returns 0 if no failures */
	return n_failed;
}

/* asks for yes/no. returns 0 if yes and 1 if no */
int pause_yn(const char* prompt){
	char c;
	int ret;

	if (!prompt){
		prompt = "Yes or no (Y/N)?";
	}

	/* clear stdin of any leftover characters */
	while ((c = getchar()) != '\n' && c != EOF);

	log_default("%s", prompt);
	c = getchar();

	switch (c){
	case 'y':
	case 'Y':
		ret = TEST_SUCCESS;
	default:
		ret = TEST_FAIL;
	}

	/* clear stdin of any trailing characters */
	while ((c = getchar()) != '\n' && c != EOF);
	return ret;
}
