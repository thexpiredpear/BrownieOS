#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <mm/paging.h>
#include <core/isr.h>
#include <drivers/tty.h>
#include <core/idt.h>
#include <core/common.h>

isr_t interrupt_handlers[256];
uint32_t* apic_eoi;

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
    // eoi apic 
    //*apic_eoi = 0;
    return;
}

// set custom isr handler
void isr_set_handler(uint8_t int_no, isr_t handler) {
    interrupt_handlers[int_no] = handler;
}

void isr_init() {
    apic_eoi = (uint32_t*)KP2V(0xFEE000B0);
}