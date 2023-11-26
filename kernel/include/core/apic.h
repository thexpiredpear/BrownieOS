#ifndef _KERNEL_APIC_H
#define _KERNEL_APIC_H 1

#include <stdint.h>
#include <stdbool.h>

bool confirm_apic();

void init_apic();

#endif