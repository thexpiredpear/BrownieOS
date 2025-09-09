#ifndef _KERNEL_GDT_H
#define _KERNEL_GDT_H 1

#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

typedef struct gdt_entry gdt_entry_t;

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

typedef struct gdt_ptr gdt_ptr_t;

// Builds a flat-segmentation GDT (kernel/user code/data descriptors) in memory
// and loads it via `gdt_flush`. Must be called before enabling interrupts.
void gdt_init();
// Populates a GDT entry at `index` with base, limit, access, and granularity
// fields. All parameters are raw descriptor fields (not pointers or addresses
// to data structures). Writes into the in-memory GDT (kernel virtual address).
void gdt_set_gate(int32_t, uint32_t, uint32_t, uint8_t, uint8_t);
// Reloads GDTR with the provided pointer (kernel virtual address to a `gdt_ptr_t`),
// and updates segment registers. Implemented in assembly and does not return
// until the new GDT is active.
extern void gdt_flush(uint32_t);

#endif
