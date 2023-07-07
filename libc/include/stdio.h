#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stddef.h>
#include <kernel/tty.h>

#define EOF (-1)

int vprintf(const char* __restrict, __builtin_va_list);
int vsnprintf(char* __restrict, size_t, const char* __restrict, __builtin_va_list);
int vsprintf(char* __restrict, const char* __restrict, __builtin_va_list);
int printf(const char* __restrict, ...);
int snprintf(char* __restrict, size_t, const char* __restrict, ...);
int sprintf(char* __restrict, const char* __restrict, ...);
int putchar(int);
int puts(const char*);

#endif
