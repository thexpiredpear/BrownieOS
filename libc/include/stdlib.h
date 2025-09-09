#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>

// Aborts execution of the current program. In the kernel build, prints a panic
// message and halts the CPU; never returns. Does not unwind or free resources.
__attribute__((__noreturn__))
// Never returns; see description above.
void abort(void);

#endif
