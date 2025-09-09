#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

// Compares `size` bytes at `a` and `b`. Returns <0, 0, >0 per lexicographic
// ordering of bytes. Pointers are raw memory (no alignment assumptions).
int memcmp(const void*, const void*, size_t);
// Copies `size` bytes from `src` to `dst`. Regions must not overlap. Returns
// `dst`. Both pointers are raw memory; uses forward copy.
void* memcpy(void* __restrict, const void* __restrict, size_t);
// Copies `size` bytes from `src` to `dst`, correctly handling overlap by
// choosing copy direction. Returns `dst`.
void* memmove(void*, const void*, size_t);
// Fills `size` bytes at `buf` with byte value `value`. Returns `buf`.
void* memset(void*, int, size_t);
// Returns the length of a NUL-terminated string `s` (excluding terminator).
size_t strlen(const char*);
// Compares strings `a` and `b`. Returns <0, 0, >0 per lexicographic order.
int strcmp(const char*, const char*);
// Compares up to `size` bytes of strings `a` and `b`. Returns <0, 0, >0.
int strncmp(const char*, const char*, size_t);

#endif
