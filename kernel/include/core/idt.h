#ifndef _KERNEL_IDT_H
#define _KERNEL_IDT_H 1

#include <stdint.h>

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t reserved;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

typedef struct idt_entry idt_entry_t;

struct idt_ptr {
    uint16_t size;
    uint32_t offset;
} __attribute__((packed));

typedef struct idt_ptr idt_ptr_t;

// Reprograms legacy PIC vectors to start at 0x20 to avoid clashing with CPU
// exceptions (0x00–0x1F). Issues outb to PIC command/data ports; no arguments.
void irq_remap();
// Initializes the IDT in memory, installs gates for exceptions/IRQs using
// ISRs/IRQs stubs, remaps PIC, and loads IDTR via `idt_flush`.
void idt_init();
// Sets a single IDT entry: `index` is the vector, `base` is the handler’s linear
// (kernel virtual) address, `selector` is the code segment selector, and `flags`
// encode type, DPL, and present bit. Writes into the in-memory IDT.
void idt_set_gate(uint8_t, uint32_t, uint16_t, uint8_t);
// Common high-level exception handler (C). Never returns; typically logs and halts.
__attribute__((noreturn)) void exception_handler();
// Loads IDTR with the given pointer to an `idt_ptr_t` (kernel virtual address).
// Implemented in assembly; activates the in-memory IDT for interrupt dispatch.
extern void idt_flush(uint32_t);

// ISR stub for CPU exception #0 (Divide Error). Transfers control to C handler.
extern void isr0(); 
// ISR stub for CPU exception #1 (Debug).
extern void isr1();
// ISR stub for CPU exception #2 (NMI Interrupt).
extern void isr2();
// ISR stub for CPU exception #3 (Breakpoint).
extern void isr3();
// ISR stub for CPU exception #4 (Overflow).
extern void isr4();
// ISR stub for CPU exception #5 (BOUND Range Exceeded).
extern void isr5();
// ISR stub for CPU exception #6 (Invalid Opcode).
extern void isr6();
// ISR stub for CPU exception #7 (Device Not Available).
extern void isr7();
// ISR stub for CPU exception #8 (Double Fault).
extern void isr8();
// ISR stub for CPU exception #9 (Coprocessor Segment Overrun / reserved).
extern void isr9();
// ISR stub for CPU exception #10 (Invalid TSS).
extern void isr10();
// ISR stub for CPU exception #11 (Segment Not Present).
extern void isr11();
// ISR stub for CPU exception #12 (Stack-Segment Fault).
extern void isr12();
// ISR stub for CPU exception #13 (General Protection Fault).
extern void isr13();
// ISR stub for CPU exception #14 (Page Fault).
extern void isr14();
// ISR stub for CPU exception #15 (reserved).
extern void isr15();
// ISR stub for CPU exception #16 (x87 FPU Floating-Point Error).
extern void isr16();
// ISR stub for CPU exception #17 (Alignment Check).
extern void isr17();
// ISR stub for CPU exception #18 (Machine Check).
extern void isr18();
// ISR stub for CPU exception #19 (SIMD Floating-Point Exception).
extern void isr19();
// ISR stub for CPU exception #20 (Virtualization Exception).
extern void isr20();
// ISR stub for CPU exception #21 (Control Protection Exception).
extern void isr21();
// ISR stub for CPU exception #22 (reserved).
extern void isr22();
// ISR stub for CPU exception #23 (reserved).
extern void isr23();
// ISR stub for CPU exception #24 (reserved).
extern void isr24();
// ISR stub for CPU exception #25 (reserved).
extern void isr25();
// ISR stub for CPU exception #26 (reserved).
extern void isr26();
// ISR stub for CPU exception #27 (reserved).
extern void isr27();
// ISR stub for CPU exception #28 (reserved).
extern void isr28();
// ISR stub for CPU exception #29 (reserved).
extern void isr29();
// ISR stub for CPU exception #30 (reserved).
extern void isr30();
// ISR stub for CPU exception #31 (reserved).
extern void isr31();

// PIC/APIC-delivered hardware interrupt 0 (IRQ0, PIT timer).
extern void irq0();
// Hardware interrupt 1 (IRQ1, keyboard controller).
extern void irq1();
// Hardware interrupt 2 (IRQ2, cascaded to slave PIC / APIC route).
extern void irq2();
// Hardware interrupt 3 (IRQ3, serial port COM2/COM4 by default).
extern void irq3();
// Hardware interrupt 4 (IRQ4, serial port COM1/COM3 by default).
extern void irq4();
// Hardware interrupt 5 (IRQ5, legacy devices / user-assigned).
extern void irq5();
// Hardware interrupt 6 (IRQ6, floppy controller by default).
extern void irq6();
// Hardware interrupt 7 (IRQ7, parallel port / spurious on master PIC).
extern void irq7();
// Hardware interrupt 8 (IRQ8, RTC on legacy systems).
extern void irq8();
// Hardware interrupt 9 (IRQ9, ACPI / redirected from IRQ2).
extern void irq9();
// Hardware interrupt 10 (IRQ10).
extern void irq10();
// Hardware interrupt 11 (IRQ11).
extern void irq11();
// Hardware interrupt 12 (IRQ12, PS/2 mouse on legacy systems).
extern void irq12();
// Hardware interrupt 13 (IRQ13, x87 FPU on legacy systems).
extern void irq13();
// Hardware interrupt 14 (IRQ14, primary ATA channel).
extern void irq14();
// Hardware interrupt 15 (IRQ15, secondary ATA channel).
extern void irq15();

// Software interrupt for syscalls (int 0x80)
extern void isr128();

#endif
