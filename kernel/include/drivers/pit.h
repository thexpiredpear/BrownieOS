#ifndef _KERNEL_PIT_H
#define _KERNEL_PIT_H

#include <stdint.h>
#include <core/isr.h>

void pit_init(uint32_t frequency);

#endif