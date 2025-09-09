#ifndef _KERNEL_HPET_H
#define _KERNEL_HPET_H 1

#include <stdint.h>
#include <stdbool.h>

// Initializes the High Precision Event Timer (HPET): discovers the HPET via
// ACPI, maps its MMIO registers into the kernel virtual address space, and
// configures a periodic timer source delivering interrupts to the LAPIC/IOAPIC
// using comparator 0. Implementation selects rate based on internal policy.
void init_hpet(uint32_t freq);

#endif
