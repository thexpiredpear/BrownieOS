#include <stdint.h>
#include <stdio.h>
#include <kernel/isr.h>
#include <kernel/tty.h>
#include <kernel/idt.h>
#include <kernel/common.h>

isr_t interrupt_handlers[256];

void page_fault(regs_t* registers) {
    printf("page fault\n");
    printf("error code: %x\n", registers->err_code);
    return;
}

// isr handler
void isr_handler(regs_t* registers) {
    isr_t handler = interrupt_handlers[registers->int_no];
    if(handler) {
        handler(registers);
    } else {
        printf("received interrupt: code %d\n", registers->int_no);
    }
    return;
}

// irq handler
void irq_handler(regs_t* registers) {
    isr_t handler = interrupt_handlers[registers->int_no];
    if(handler) {
        handler(registers);
    } else {
        printf("received interrupt: code %d\n", registers->int_no);
    }
    // eoi slave
    if(registers->int_no >= 40) {
        outb(0xA0, 0x20);
    }
    // eoi master
    outb(0x20, 0x20);
    return;
}

// set custom isr handler
void isr_set_handler(uint8_t int_no, isr_t handler) {
    interrupt_handlers[int_no] = handler;
}