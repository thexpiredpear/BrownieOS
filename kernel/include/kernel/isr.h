#ifndef _KERNEL_ISR_H
#define _KERNEL_ISR_H 1

#include <stdint.h>

typedef struct regs {
    uint16_t gs, fs, es, ds; // PUSHED MANUALLY
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // PUSHAD
    uint32_t int_no, err_code;  // PUSHED BY ISR
    uint32_t eip, cs, eflags; // PUSHED BY CPU
} regs_t;

typedef void (*isr_t)(regs_t*); // function pointer to custom isr handler

void isr_set_handler(uint8_t, isr_t);
void isr_handler(regs_t*);
void irq_handler(regs_t*);

#endif