#ifndef _KERNEL_ISR_H
#define _KERNEL_ISR_H 1

#include <stdint.h>

struct int_regs {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // PUSHAD
    uint32_t int_no, err_code;  // PUSHED BY ISR
    uint32_t eip, cs, eflags; // PUSHED BY CPU
} __attribute__((packed));

typedef struct int_regs int_regs_t;

// Function pointer type for per-vector handlers installed by the kernel. The
// handler is passed a pointer to the saved CPU register frame for that fault or
// interrupt. Pointer is a KERNEL virtual address to a transient stack frame.
typedef void (*isr_t)(int_regs_t*); // function pointer to custom isr handler

// Installs a custom high-level handler for vector `uint8_t`. The handler will
// be called from `isr_handler`/`irq_handler` after the low-level stub saves the
// CPU state. Overwrites any previous handler.
void isr_set_handler(uint8_t, isr_t);
// Top-level handler for exceptions (ISRs 0–31). Dispatches to any registered
// `isr_t` for the incoming vector; otherwise logs default information.
void isr_handler(int_regs_t*);
// Top-level handler for hardware IRQs (after PIC/APIC remap). Acknowledges the
// interrupt controller as needed and dispatches to any registered `isr_t`.
void irq_handler(int_regs_t*);

// Initializes ISR infrastructure that requires runtime setup (e.g., maps APIC
// EOI register to write end-of-interrupts). Does not modify the IDT itself.
void isr_init();

#endif
