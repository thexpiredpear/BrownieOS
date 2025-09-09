#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H 1

#include <stdint.h>
#include <stdbool.h>
#include <core/multiboot.h>
#include <core/isr.h>

#define PAGE_SIZE (0x1000)
#define PAGE_TABLE_SIZE (0x400000)
#define EOM (0xFFFFFFFF)
#define KERN_START_TBL (768)
#define KERN_HIGHMEM_START_TBL (992)

#define PAGE_FAULT_PRESENT_A (0b1)
#define PAGE_FAULT_WRITE_A (0b10)
#define PAGE_FAULT_USER_A (0b100)
#define PAGE_FAULT_RESERVED_A (0b1000)

#define PAGE_FRAME(x) ((x) / 0x1000)
#define PAGE_FRAME_BITMAP_IDX(x) ((x) / 8)
#define PAGE_FRAME_BITMAP_OFF(x) ((x) % 8)

#define PAGE_PADDR(x) ((x) * 0x1000)

#define PAGE_ROUND_DOWN(x) ((x) & 0xFFFFF000);
#define PAGE_ROUND_UP(x) (((x) % 0x1000) ? (((x) & 0xFFFFF000) + 0x1000) : (x))

#define PAGE_DIR_IDX(x) ((uint32_t)(x) / 0x400000)
#define PAGE_TBL_IDX(x) (((uint32_t)(x) % 0x400000) / 0x1000)

#define PAGE_IDX_VADDR(d, t, o) (((d) * 0x400000) + ((t) * 0x1000) + (o))

#define NFRAMES ((PAGE_FRAME(EOM)) + 1)

#define KV2P(x) ((uint32_t)(x) - 0xC0000000)
#define KP2V(x) ((uint32_t)(x) + 0xC0000000)

struct page {
    uint32_t present    : 1;   // Present in memory if set
    uint32_t rw         : 1;   // Readwrite if set
    uint32_t user       : 1;   // User mode if set
    uint32_t accessed   : 1;   // Has the page been accessed since last refresh?
    uint32_t dirty      : 1;   // Has the page been written to since last refresh?
    uint32_t unused     : 7;   // Unused / reserved bits
    uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
} __attribute__((packed));

typedef struct page page_t;

typedef uint8_t pmm_flags_t;

#define PMM_FLAGS_DEFAULT ((uint8_t)0)
#define PMM_FLAGS_HIGHMEM ((uint8_t)0b1)

struct page_dir_entry {
    uint32_t present    : 1;   // Present in memory if set
    uint32_t rw         : 1;   // Readwrite if set
    uint32_t user       : 1;   // User mode if set
    uint32_t write_thru : 1;   // Write through cache if set
    uint32_t disable_cache : 1;   // Disable cache if set
    uint32_t accessed   : 1;   // Has the page been accessed since last refresh?
    uint32_t reserved   : 1;   // Reserved
    uint32_t page_size  : 1;   // Page size (0 = 4kb, 1 = 4mb)
    uint32_t unused     : 4;   // Unused / reserved bits
    uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
} __attribute__((packed));
    
typedef struct page_dir_entry page_dir_entry_t;

struct page_table {
    page_t pages[1024];
} __attribute__((packed)) __attribute__((aligned(0x1000)));

typedef struct page_table page_table_t;

struct page_directory {
    // page entries in page_dir_entry and tables are identical
    page_dir_entry_t page_dir_entries[1024]; // dir entries, physical table addresses for paging
    page_table_t* tables[1024]; // virtual addresses for r/w access to tables - NO PARAMS
} __attribute__((packed)) __attribute__((aligned(0x1000)));

typedef struct page_directory page_directory_t;

// Handles CPU exception 14 (page fault). Reads CR2 to obtain the faulting
// linear address (virtual address), decodes the error code in the provided
// `int_regs_t` (pushed by the ISR stub), and reports whether the fault was due
// to not-present, write, user-mode, or reserved-bit violations. Does not return.
void page_fault(int_regs_t*);

// Marks the 4 KiB physical frame containing physical address `addr` as used in
// the physical frame bitmap (global to the PMM). `addr` is a physical address.
void set_frame(uint32_t addr);
// Marks the 4 KiB physical frame containing physical address `addr` as free in
// the physical frame bitmap (global to the PMM). `addr` is a physical address.
void clear_frame(uint32_t addr);
// Returns whether the 4 KiB physical frame containing physical address `addr`
// is currently marked used in the physical frame bitmap. `addr` is physical.
bool test_frame(uint32_t addr);

// Allocates `count` contiguous 4 KiB physical frames and marks them used in the
// global frame bitmap. Flags: PMM_FLAGS_DEFAULT searches only "lowmem"
// (phys < KERN_HIGHMEM_START_TBL*PAGE_TABLE_SIZE), which the kernel identity-maps;
// PMM_FLAGS_HIGHMEM searches only "highmem" (phys >= that boundary), intended for
// user pages or large buffers temporarily mapped via kmap(). Returns the base
// PHYSICAL address (PAGE_SIZE-aligned) of the first frame, or 0 on failure; no
// virtual mapping is created.
uint32_t alloc_pages(pmm_flags_t flags, uint32_t count);
// Frees `count` contiguous 4 KiB physical frames starting at frame number
// `frame` (i.e., the physical address is `frame * PAGE_SIZE`). Updates only the
// frame bitmap; does not unmap any existing virtual mappings.
void free_pages(uint32_t frame, uint32_t count);

// Switches the active page directory by loading CR3 with the PHYSICAL address
// of `dir` (passed as a kernel virtual pointer). Implicitly invalidates the
// TLB as part of moving CR3. Affects all subsequent address translations.
void swap_dir(page_directory_t* dir);
// Invalidates the TLB for the current address space by reloading CR3 with its
// current value. Does not modify any page structures; purely a hardware flush.
void flush_tlb(void);

// Marks a contiguous physical region [start, start+length) as reserved in the
// frame bitmap. Rounds to page boundaries. Inputs are PHYSICAL byte addresses.
void reserve(uint32_t start, uint32_t length);

// Copies raw PTE bitfields (page_t entries) from `src` to `dest` without
// allocating or modifying backing physical frames. Both pointers are KERNEL
// virtual addresses to page table structures (not the hardware table address).
void copy_page_table_entries(page_table_t* src, page_table_t* dest);

// Creates a logical clone of `src` into `dest`. Kernel entries (>= KERN_START_TBL)
// are shared by reference so they point to the same physical frames. User-space
// entries are deep-copied: for each mapped page, a new PHYSICAL frame is
// allocated and contents copied via a temporary kernel mapping. Both pointers
// are kernel virtual addresses to page_directory structures.
void clone_page_dir(page_directory_t* src, page_directory_t* dest);

// Writes a single page table entry `*page` (a software view of a PTE).
// The `frame` parameter is the PHYSICAL FRAME NUMBER (i.e., physical address
// >> 12), not a pointer. Flags control presence, writeability, and privilege.
void set_page(page_t* page, uint32_t frame, bool present, bool rw, bool user);

// Initializes the paging subsystem: installs the page-fault handler, builds the
// kernel page directory and low-memory identity mappings (via kernel virtual
// window), loads CR3, and reserves non-available regions from the Multiboot
// memory map. `mbd` is a PHYSICAL pointer from the bootloader; it is adjusted
// to a kernel virtual address internally.
void paging_init(multiboot_info_t* mbd, uint32_t magic);


#endif
