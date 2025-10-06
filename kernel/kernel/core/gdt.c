#include <stdio.h>
#include <stdint.h>
#include <drivers/tty.h>
#include <core/gdt.h>
#include <core/tss.h>

gdt_entry_t gdt_entries[6];
gdt_ptr_t gdt_ptr;

void gdt_init() {
    // Grow GDT to include a TSS descriptor (index 5)
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base  = (uint32_t)(&gdt_entries);

    // FLAT SEGMENTATION MODEL (0x00 - 0xFFFFFFFF)
    gdt_set_gate(0, 0, 0, 0, 0); // Null segment
    gdt_set_gate(1, 0, 0xFFFFFF, 0b10011010, 0b11001111); // Code segment
    gdt_set_gate(2, 0, 0xFFFFFF, 0b10010010, 0b11001111); // Data segment
    gdt_set_gate(3, 0, 0xFFFFFF, 0b11111010, 0b11001111); // User mode code segment
    gdt_set_gate(4, 0, 0xFFFFFF, 0b11110010, 0b11001111); // User mode data segment

    // Load GDT first so we can safely install and load the TSS next
    gdt_flush((uint32_t)(&gdt_ptr));

    // Initialize and load the Task State Segment (provides SS0/ESP0 for CPL3->0 transitions)
    tss_init();
}

void gdt_set_gate(int32_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt_entries[index].base_low = (uint16_t)(base & 0xFFFF);
    gdt_entries[index].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt_entries[index].base_high = (uint8_t)((base >> 24) & 0xFF);

    gdt_entries[index].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt_entries[index].granularity = (uint8_t)((flags & 0xF0) | ((limit >> 16) & 0xF));

    gdt_entries[index].access = access;
}
