#ifndef __TEST_BASE_H
#define __TEST_BASE_H

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

void printf_red(const char* format, ...);
void printf_yellow(const char* format, ...);
void printf_green(const char* format, ...);
void printf_blue(const char* format, ...);
void __massert(int condition, const char* file, int line, const char* msg);
#define massertm(condition, msg) __massert((int)(condition), __FILE__, __LINE__, msg)
#define massert(condition) __massert((intptr_t)(condition), __FILE__, __LINE__, #condition)

#endif
