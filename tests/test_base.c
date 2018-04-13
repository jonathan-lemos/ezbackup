/* test_base.c -- helper functions for tests
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "test_base.h"
#include "../error.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>

void printf_red(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	printf("\033[91m");
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
	printf("\033[92m");
	vprintf(format, ap);
	printf("\033[m");
	fflush(stdout);
	va_end(ap);
}

void printf_blue(const char* format, ...){
	va_list ap;
	va_start(ap, format);
	printf("\033[94m");
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
		log_debug(__FL__, "Failed to create_file() (%s)", strerror(errno));
		return;
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
