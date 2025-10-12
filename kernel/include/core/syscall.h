#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H 1

#include <stdint.h>
#include <core/isr.h>

// Software interrupt vector reserved for syscalls (int 0x80).
#define SYSCALL_VECTOR 0x80
// Maximum number of syscall IDs supported by the kernel dispatcher.
#define SYSCALL_MAX 256

typedef void (*syscall_handler_t)(int_regs_t* regs);

// Initializes the syscall dispatcher and hooks vector 0x80 into the IDT.
void syscall_init(void);
// Registers a handler for the syscall number `num`.
int syscall_register(uint32_t num, syscall_handler_t handler);

#endif
