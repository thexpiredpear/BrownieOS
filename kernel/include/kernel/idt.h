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

void irq_remap();
void idt_init();
void idt_set_gate(uint8_t, uint32_t, uint16_t, uint8_t);
__attribute__((noreturn)) void exception_handler();
extern void idt_flush(uint32_t);

extern void isr0(); 
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

#endif