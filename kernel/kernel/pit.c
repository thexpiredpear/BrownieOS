#include <stdint.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/isr.h>
#include <kernel/pit.h>
#include <kernel/common.h>

uint32_t tick = 0;

void pit_handler(regs_t* registers) {
    tick++;
    printf("Tick: %d Signal:%d  \n ", tick, registers->int_no);
    return;
}

void pit_init(uint32_t frequency) {
    isr_set_handler(32, &pit_handler);
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);
    outb(0x40, l);
    outb(0x40, h);
    return;
}
