#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stddef.h>
#include <drivers/tty.h>

#define EOF (-1)

// Converts signed integer `i` to a NUL-terminated string in `str` using `base`
// (2..36). Returns `str`. Does not allocate; caller provides buffer.
char* itoa(int i, char* str, int base);
// Converts unsigned integer `i` to string in `str` using `base` (2..36).
char* utoa(unsigned int i, char* str, int base);
// Converts signed long `i` to string in `str` using `base` (2..36).
char* ltoa(long i, char* str, int base);
// Converts unsigned long `i` to string in `str` using `base` (2..36).
char* ultoa(unsigned long i, char* str, int base);
// Converts signed long long `i` to string in `str` using `base` (2..36).
char* lltoa(long long i, char* str, int base);
// Converts unsigned long long `i` to string in `str` using `base` (2..36).
char* ulltoa(unsigned long long i, char* str, int base);

// Formats and writes to the kernel TTY using a va_list. Supports %c, %s, %d,
// %i, %x, and literal %%. Returns number of characters written.
int vprintf(const char* __restrict, __builtin_va_list);
// Formats into buffer `char*` with size limit `size` using a va_list. Returns
// number of characters written excluding the NUL terminator.
int vsnprintf(char* __restrict, size_t, const char* __restrict, __builtin_va_list);
// Formats into buffer without size limit (delegates to `vsnprintf`).
int vsprintf(char* __restrict, const char* __restrict, __builtin_va_list);
// Formats and writes to the kernel TTY. Returns number of characters written.
int printf(const char* __restrict, ...);
// Formats into buffer with size limit `n`. Returns characters written.
int snprintf(char* __restrict, size_t, const char* __restrict, ...);
// Formats into buffer without size limit. Returns characters written.
int sprintf(char* __restrict, const char* __restrict, ...);
// Writes a single character to the active terminal backend (VGA text mode).
int putchar(int);
// Writes a NUL-terminated string followed by a newline via `printf`.
int puts(const char*);

#endif
