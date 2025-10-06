#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H 1

#include <stdint.h>

// Syscall numbers (private ABI for BrownieOS demo)
enum syscall_no {
    SYS_write  = 1,
    SYS_brk    = 2,
    SYS_sbrk   = 3,
    SYS_getpid = 4,
    SYS_exit   = 5,
    SYS_fork   = 6,
};

// Installs the syscall handler on vector 0x80.
void syscall_init(void);

#endif

