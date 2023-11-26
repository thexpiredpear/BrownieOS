#include <cpuid.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <core/apic.h>
#include <core/acpi.h>
#include <core/multiboot.h>
#include <core/common.h>
#include <mm/vmm.h>
#include <mm/kmm.h>

#define IA32_APIC_BASE_MSR 0x1B

bool confirm_apic() {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    return edx & CPUID_FEAT_EDX_APIC;
}

void init_apic() {
    if(!confirm_apic()) {
        panic("apic not available");
    }
}