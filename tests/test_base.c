#include "test_base.h"
#include <execinfo.h>
#include <stdlib.h>
#include <signal.h>

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
	printf("%s", msg);
	printf("\nCall stack:\n");

	abort();
}

static void handle_signals(int signo){
	void* callstack[256];
	char** strs;
	int frames;
	int i;

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
		printf_blue("Unknown signal\n");
	}

	frames = backtrace(callstack, sizeof(callstack) / sizeof(callstack[0]));
	strs = backtrace_symbols(callstack, frames);
	for (i = 0; i < frames; ++i){
		printf("%s\n", strs[i]);
	}
	free(strs);

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
	massert(fp);
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

	if (c1 != c2){
		return c1 - c2;
	}

	return 0;
}
