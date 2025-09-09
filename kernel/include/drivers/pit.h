#ifndef _KERNEL_PIT_H
#define _KERNEL_PIT_H

#include <stdint.h>
#include <core/isr.h>

// Programs the legacy PIT (8253/8254) to generate periodic IRQ0 at the desired
// `frequency` (Hz): installs an ISR for vector 32, configures PIT mode and
// divisor via I/O ports 0x43/0x40. `frequency` is a scalar (not a time period).
void pit_init(uint32_t frequency);

#endif
