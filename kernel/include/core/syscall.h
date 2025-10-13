#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H 1

#include <stdint.h>
#include <core/isr.h>

// Software interrupt vector reserved for syscalls (int 0x80).
#define SYSCALL_VECTOR (0x80)
// Maximum number of syscall IDs supported by the kernel dispatcher.
#define SYSCALL_MAX (256)

#define SYSCALL_SUCCESS (0)
#define SYSCALL_EINVAL (-22)
#define SYSCALL_EFAULT (-14)

// Temporary syscall numbers for MVP userland interactions.
#define SYS_PRINT_STRING (0x1)

typedef void (*syscall_handler_t)(int_regs_t* regs);

// Checks if a virtual address within a process' page directory is accessible by user code.
bool user_addr_accessible(const proc_t* proc, uint32_t addr);
// Initializes the syscall dispatcher and hooks vector 0x80 into the IDT.
void syscall_init(void);
// Registers a handler for the syscall number `num`.
int syscall_register(uint32_t num, syscall_handler_t handler);
void syscall_dispatch(int_regs_t* regs);
void sys_print_string(int_regs_t* regs);

#endif
