#ifndef _KERNEL_TSS_H
#define _KERNEL_TSS_H 1

#include <stdint.h>

// 32-bit i386 Task State Segment layout used for privilege transitions.
// We only rely on ss0/esp0 for ring3->ring0 stack switching on interrupts.
typedef struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;    // Ring 0 stack pointer
    uint32_t ss0;     // Ring 0 stack segment
    uint32_t esp1;    // Unused
    uint32_t ss1;     // Unused
    uint32_t esp2;    // Unused
    uint32_t ss2;     // Unused
    uint32_t cr3;     // Unused
    uint32_t eip;     // Unused
    uint32_t eflags;  // Unused
    uint32_t eax;     // Unused
    uint32_t ecx;     // Unused
    uint32_t edx;     // Unused
    uint32_t ebx;     // Unused
    uint32_t esp;     // Unused
    uint32_t ebp;     // Unused
    uint32_t esi;     // Unused
    uint32_t edi;     // Unused
    uint16_t es;      // Unused
    uint16_t cs;      // Unused
    uint16_t ss;      // Unused
    uint16_t ds;      // Unused
    uint16_t fs;      // Unused
    uint16_t gs;      // Unused
    uint16_t ldt;     // Unused
    uint16_t trap;    // Unused (bit 0 = 1 to trigger debug trap on task switch)
    uint16_t iomap_base; // Offset to I/O permission bitmap; set to sizeof(tss)
} __attribute__((packed)) tss_entry_t;

// Writes the TSS descriptor into the GDT (index 5, selector 0x28), initializes
// ss0/esp0, and loads TR via ltr. Uses an internal kernel-only stack initially.
void tss_init(void);
// Updates the kernel stack pointer used on privilege elevation.
void tss_set_kernel_stack(uint32_t kstack_top);

// Assembly helper that loads TR with the given selector (e.g., 0x28).
extern void tss_flush(uint16_t selector);

#endif

