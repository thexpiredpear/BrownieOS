#ifndef _KERNEL_APIC_H
#define _KERNEL_APIC_H 1

#include <stdint.h>
#include <stdbool.h>

void disable_pic();
uint32_t get_apic_base();
bool confirm_apic();
void init_apic();

#endif