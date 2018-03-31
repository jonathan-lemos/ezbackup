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
	void* callstack[256];
	char** strs;
	int frames;
	int i;

	if (condition){
		return;
	}

	printf("Assertion Failed (%s:%d): ", file, line);
	printf("%s", msg);
	printf("\nCall stack:\n");

	frames = backtrace(callstack, sizeof(callstack) / sizeof(callstack[0]));
	strs = backtrace_symbols(callstack, frames);
	for (i = 0; i < frames; ++i){
		printf("%s\n", strs[i]);
	}
	free(strs);

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
			printf_blue("Unknown signal\n");
	}
	exit(1);
}

void set_signal_handler(void){
	struct sigaction sa;
	sa.sa_handler = handle_signals;
	sigfillset(&(sa.sa_mask));
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
