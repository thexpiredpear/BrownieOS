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

void page_fault(int_regs_t*);

void set_frame(uint32_t addr);
void clear_frame(uint32_t addr);
bool test_frame(uint32_t addr);

uint32_t alloc_pages(pmm_flags_t flags, uint32_t count);
void free_pages(uint32_t frame, uint32_t count);

void swap_dir(page_directory_t* dir);
void flush_tlb(void);

void reserve(uint32_t start, uint32_t length);

void copy_page_table_entries(page_table_t* src, page_table_t* dest);

void set_page(page_t* page, uint32_t frame, bool present, bool rw, bool user);

void paging_init(multiboot_info_t* mbd, uint32_t magic);


#endif