#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <core/syscall.h>
#include <core/isr.h>

static syscall_handler_t syscall_table[SYSCALL_MAX];

static void syscall_dispatch(int_regs_t* regs);

void syscall_init(void) {
    memset(syscall_table, 0, sizeof(syscall_table));
    isr_set_handler(SYSCALL_VECTOR, syscall_dispatch);
}

int syscall_register(uint32_t num, syscall_handler_t handler) {
    if(num >= SYSCALL_MAX || handler == NULL) {
        return -1;
    }
    if(syscall_table[num] != NULL) {
        return -2;
    }

    syscall_table[num] = handler;
    return 0;
}

static void syscall_dispatch(int_regs_t* regs) {
    if(regs == NULL) {
        return;
    }

    uint32_t num = regs->eax;
    if(num < SYSCALL_MAX) {
        syscall_handler_t handler = syscall_table[num];
        if(handler != NULL) {
            handler(regs);
            return;
        }
    }

    regs->eax = (uint32_t)-1;
    printf("syscall: unhandled number %u\n", num);
}
