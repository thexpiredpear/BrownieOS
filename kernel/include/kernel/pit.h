#ifndef _KERNEL_PIT_H
#define _KERNEL_PIT_H

#include <stdint.h>
#include <kernel/isr.h>

void pit_init(uint32_t frequency);

#endif