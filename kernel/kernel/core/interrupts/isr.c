#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <core/isr.h>
#include <drivers/tty.h>
#include <core/idt.h>
#include <core/common.h>

isr_t interrupt_handlers[256];

// isr handler
void isr_handler(int_regs_t* registers) {
    isr_t handler = interrupt_handlers[registers->int_no];
    if(handler) {
        handler(registers);
    } else {
        printf("received interrupt: code %d\n", registers->int_no);
    }
    return;
}

// irq handler
void irq_handler(int_regs_t* registers) {
    isr_t handler = interrupt_handlers[registers->int_no];
    if(handler) {
        handler(registers);
    } else {
        printf("received interrupt: code %d\n", registers->int_no);
    }
    // eoi slave pic
    if(registers->int_no >= 40) {
        outb(0xA0, 0x20);
    }
    // eoi master pic
    outb(0x20, 0x20);
    return;
}

// set custom isr handler
void isr_set_handler(uint8_t int_no, isr_t handler) {
    interrupt_handlers[int_no] = handler;
}