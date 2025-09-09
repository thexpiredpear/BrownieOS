#ifndef _KERNEL_COMMON_H
#define _KERNEL_COMMON_H 1

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
// ordered array impl, order on insertion

typedef bool(*predicate_t)(uint32_t, uint32_t);

struct ordered_array {
    uint32_t* array;
    uint32_t size;
    uint32_t max_size;  
    predicate_t predicate;
};

typedef struct ordered_array ordered_array_t;

// ordered array functions

// Creates an ordered array backed by pre-provided memory at kernel virtual
// address `addr` with capacity `max_size`. Initializes slots to zero and uses
// `less_predicate` for ordering. Caller manages lifetime of backing storage.
ordered_array_t init_ordered_array_place(void* addr, uint32_t max_size);
// Inserts `val` into the array in order (stable insertion). Returns the index
// where the value was inserted. Fails if the array is at maximum capacity.
uint32_t insert_ordered_array(ordered_array_t* ordered_array, uint32_t val);
// Removes the element at index `i`, shifting subsequent elements left by one.
// Decrements size; does not shrink the underlying buffer.
void remove_ordered_array(ordered_array_t* ordered_array, uint32_t i);
// Returns the element at index `i` without removing it.
uint32_t get_ordered_array(ordered_array_t* ordered_array, uint32_t i);
// Returns the index of the first element equal to `val`, or 0xFFFFFFFF if not found.
uint32_t find_ordered_array(ordered_array_t* ordered_array, uint32_t val);
// void destroy_ordered_array(ordered_array_t* array);

// Default ordering predicate for the ordered array (a < b).
bool less_predicate(uint32_t a, uint32_t b);
// Prints `str` padded with leading zeros up to length `len` using the kernel TTY.
void print_with_leading_zeros(uint32_t len, char* str);
// Triggers an interrupt vector (placeholder in current implementation).
void trigger_interrupt(uint8_t i);
// Intentionally dereferences NULL to trigger a page fault for testing.
void trigger_page_fault();
// Writes an 8-bit value to I/O `port` via the x86 `outb` instruction.
void outb(uint16_t port, uint8_t val);
// Reads an 8-bit value from I/O `port` via the x86 `inb` instruction.
uint8_t inb(uint16_t port);
// Reads a 16-bit value from I/O `port` via the x86 `inw` instruction.
uint16_t inw(uint16_t port);
// Clears the interrupt flag in EFLAGS (disables maskable interrupts).
void cli();
// Sets the interrupt flag in EFLAGS (enables maskable interrupts).
void sti();
// Reads a model-specific register. `msr` is the index; returns value via `lo`/`hi`
// pointers (kernel virtual addresses) representing the 64-bit MSR split in two.
void get_msr(uint32_t msr, uint32_t* lo, uint32_t* hi);
// Writes a model-specific register `msr` with the 64-bit value composed from
// `lo` (low 32 bits) and `hi` (high 32 bits).
void set_msr(uint32_t msr, uint32_t lo, uint32_t hi);
// Prints a panic message and halts the CPU. Does not return.
void panic(char* message);
// Convenience hook for breakpoints/debugging. May emit a message or trap.
void gdb_stop(void);

enum {
    CPUID_FEAT_ECX_SSE3         = 1 << 0, 
    CPUID_FEAT_ECX_PCLMUL       = 1 << 1,
    CPUID_FEAT_ECX_DTES64       = 1 << 2,
    CPUID_FEAT_ECX_MONITOR      = 1 << 3,  
    CPUID_FEAT_ECX_DS_CPL       = 1 << 4,  
    CPUID_FEAT_ECX_VMX          = 1 << 5,  
    CPUID_FEAT_ECX_SMX          = 1 << 6,  
    CPUID_FEAT_ECX_EST          = 1 << 7,  
    CPUID_FEAT_ECX_TM2          = 1 << 8,  
    CPUID_FEAT_ECX_SSSE3        = 1 << 9,  
    CPUID_FEAT_ECX_CID          = 1 << 10,
    CPUID_FEAT_ECX_SDBG         = 1 << 11,
    CPUID_FEAT_ECX_FMA          = 1 << 12,
    CPUID_FEAT_ECX_CX16         = 1 << 13, 
    CPUID_FEAT_ECX_XTPR         = 1 << 14, 
    CPUID_FEAT_ECX_PDCM         = 1 << 15, 
    CPUID_FEAT_ECX_PCID         = 1 << 17, 
    CPUID_FEAT_ECX_DCA          = 1 << 18, 
    CPUID_FEAT_ECX_SSE4_1       = 1 << 19, 
    CPUID_FEAT_ECX_SSE4_2       = 1 << 20, 
    CPUID_FEAT_ECX_X2APIC       = 1 << 21, 
    CPUID_FEAT_ECX_MOVBE        = 1 << 22, 
    CPUID_FEAT_ECX_POPCNT       = 1 << 23, 
    CPUID_FEAT_ECX_TSC          = 1 << 24, 
    CPUID_FEAT_ECX_AES          = 1 << 25, 
    CPUID_FEAT_ECX_XSAVE        = 1 << 26, 
    CPUID_FEAT_ECX_OSXSAVE      = 1 << 27, 
    CPUID_FEAT_ECX_AVX          = 1 << 28,
    CPUID_FEAT_ECX_F16C         = 1 << 29,
    CPUID_FEAT_ECX_RDRAND       = 1 << 30,
    CPUID_FEAT_ECX_HYPERVISOR   = 1 << 31,
 
    CPUID_FEAT_EDX_FPU          = 1 << 0,  
    CPUID_FEAT_EDX_VME          = 1 << 1,  
    CPUID_FEAT_EDX_DE           = 1 << 2,  
    CPUID_FEAT_EDX_PSE          = 1 << 3,  
    CPUID_FEAT_EDX_TSC          = 1 << 4,  
    CPUID_FEAT_EDX_MSR          = 1 << 5,  
    CPUID_FEAT_EDX_PAE          = 1 << 6,  
    CPUID_FEAT_EDX_MCE          = 1 << 7,  
    CPUID_FEAT_EDX_CX8          = 1 << 8,  
    CPUID_FEAT_EDX_APIC         = 1 << 9,  
    CPUID_FEAT_EDX_SEP          = 1 << 11, 
    CPUID_FEAT_EDX_MTRR         = 1 << 12, 
    CPUID_FEAT_EDX_PGE          = 1 << 13, 
    CPUID_FEAT_EDX_MCA          = 1 << 14, 
    CPUID_FEAT_EDX_CMOV         = 1 << 15, 
    CPUID_FEAT_EDX_PAT          = 1 << 16, 
    CPUID_FEAT_EDX_PSE36        = 1 << 17, 
    CPUID_FEAT_EDX_PSN          = 1 << 18, 
    CPUID_FEAT_EDX_CLFLUSH      = 1 << 19, 
    CPUID_FEAT_EDX_DS           = 1 << 21, 
    CPUID_FEAT_EDX_ACPI         = 1 << 22, 
    CPUID_FEAT_EDX_MMX          = 1 << 23, 
    CPUID_FEAT_EDX_FXSR         = 1 << 24, 
    CPUID_FEAT_EDX_SSE          = 1 << 25, 
    CPUID_FEAT_EDX_SSE2         = 1 << 26, 
    CPUID_FEAT_EDX_SS           = 1 << 27, 
    CPUID_FEAT_EDX_HTT          = 1 << 28, 
    CPUID_FEAT_EDX_TM           = 1 << 29, 
    CPUID_FEAT_EDX_IA64         = 1 << 30,
    CPUID_FEAT_EDX_PBE          = 1 << 31
};

#endif
