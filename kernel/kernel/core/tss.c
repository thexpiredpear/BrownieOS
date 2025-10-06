#include <stdint.h>
#include <string.h>
#include <core/tss.h>
#include <core/gdt.h>

// Global TSS instance
static tss_entry_t tss;

// Fallback kernel stack for early user->kernel transitions before per-process
// kernel stacks are installed. 8 KiB is sufficient for simple handlers.
static uint8_t initial_kstack[8192] __attribute__((aligned(16)));

void tss_set_kernel_stack(uint32_t kstack_top) {
    tss.esp0 = kstack_top;
}

void tss_init(void) {
    memset(&tss, 0, sizeof(tss));
    // Use kernel data segment (0x10) as the Ring 0 stack segment
    tss.ss0 = 0x10;
    // Point ESP0 at the top of an initial kernel-only stack
    tss.esp0 = (uint32_t)initial_kstack + sizeof(initial_kstack);
    // No I/O bitmap; place iomap beyond the TSS limit (disables I/O from user)
    tss.iomap_base = sizeof(tss_entry_t);

    // Install the TSS descriptor at GDT index 5 (selector 0x28):
    // access = 0x89 (Present=1, DPL=0, Type=0x9 = available 32-bit TSS)
    // flags  = 0x00 (byte granularity for system segment)
    gdt_set_gate(5, (uint32_t)&tss, sizeof(tss_entry_t) - 1, 0x89, 0x00);

    // Load the Task Register with our TSS selector (index 5 -> 0x28)
    tss_flush(0x28);
}

