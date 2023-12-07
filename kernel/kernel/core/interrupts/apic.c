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

void disable_pic() {
    // irqs already remapped in case of spurious interrupt
    outb(0xA1, 0xFF);
    outb(0x21, 0xFF);
}

uint32_t get_apic_base() {
    uint32_t lo;
    uint32_t hi;
    get_msr(IA32_APIC_BASE_MSR, &lo, &hi);
    uint64_t reg = ((uint64_t)hi << 32) | lo;
    uint32_t apic_base = ((reg >> 12) & 0xFFFFFF) << 12;
    return apic_base;
}

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
    disable_pic();
    void* apic_base = access_paddr_DANGER(get_apic_base());
    // write to spurious interrupt register to enable apic
    volatile uint32_t* spurious = (uint32_t*)((uint32_t)apic_base + 0xF0);
    *spurious = *spurious | 0x1FF;
    printf("%x\n", *spurious);
    uint32_t lo = 0;
    uint32_t hi = 0;
    get_msr(IA32_APIC_BASE_MSR, &lo, &hi);
    lo |= (1 << 11);
    set_msr(IA32_APIC_BASE_MSR, lo, hi);
}   