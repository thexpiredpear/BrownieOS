#include <stdint.h>
#include <stdio.h>
#include <drivers/tty.h>
#include <core/isr.h>
#include <drivers/pit.h>
#include <core/common.h>

double pit_osc_frequency = 3579545.0 / 3.0;
uint64_t tick = 0;

void pit_handler(int_regs_t* registers) {
    tick++;
    return;
}

void pit_init(uint32_t freq) {
    cli();
    pit_cur_frequency = freq;
    isr_set_handler(32, pit_handler);
    uint32_t divisor = (uint32_t)(pit_osc_frequency / (double)freq);
    // BCD/Binary mode: 16 bit
    // Operating mode: square wave generator
    // Access mode: low byte/high byte
    // Channel: 0
    outb(0x43, 0b00110110);
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);
    outb(0x40, l);
    outb(0x40, h);
    sti();
    return;
}
