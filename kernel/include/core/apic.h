#ifndef _KERNEL_APIC_H
#define _KERNEL_APIC_H 1

#include <stdint.h>
#include <stdbool.h>

// Masks both PICs (8259A) to prevent legacy IRQs after APIC is enabled.
void disable_pic();
// Reads MSR IA32_APIC_BASE and returns the PHYSICAL MMIO base address of the
// local APIC (aligned to 4 KiB). Does not map it.
uint32_t get_apic_base();
// Uses CPUID feature flags to determine whether the CPU supports a local APIC.
bool confirm_apic();
// Initializes the local APIC: disables PIC, maps LAPIC base via `kmap`, enables
// the APIC in the spurious interrupt register/MSR, and initializes the IOAPIC.
void init_apic();

#endif
