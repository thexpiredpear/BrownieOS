#include <stdio.h>
#include <stdint.h>
#include <kernel/tty.h>
#include <kernel/gdt.h>

gdt_entry_t gdt_entries[5];
gdt_ptr_t   gdt_ptr;

void gdt_init() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
    gdt_ptr.base  = (uint32_t)(&gdt_entries);

    // FLAT SEGMENTATION MODEL (0x00 - 0xFFFFFFFF)
    gdt_set_gate(0, 0, 0, 0, 0); // Null segment
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xCF); // Code segment
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xCF); // Data segment
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xCF); // User mode code segment
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xCF); // User mode data segment

    gdt_flush((uint32_t)(&gdt_ptr));
}

void gdt_set_gate(int32_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt_entries[index].base_low = (uint16_t)(base & 0xFFFF);
    gdt_entries[index].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt_entries[index].base_high = (uint8_t)((base >> 24) & 0xFF);

    gdt_entries[index].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt_entries[index].granularity = (uint8_t)((flags & 0xF0) | ((limit >> 16) & 0xF));

    gdt_entries[index].access = access;
}