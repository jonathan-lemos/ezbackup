#include "test_base.h"
#include <execinfo.h>
#include <stdlib.h>

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
