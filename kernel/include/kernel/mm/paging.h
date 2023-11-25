#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H 1

#include <stdint.h>
#include <stdbool.h>
#include <kernel/multiboot.h>
#include <kernel/isr.h>

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
} __attribute__((packed));

typedef struct page_table page_table_t;

struct page_directory {
    // page entries in page_dir_entry and tables are identical
    page_dir_entry_t page_dir_entries[1024]; // dir entries, physical table addresses for paging
    page_table_t* tables[1024]; // virtual addresses for r/w access to tables - NO PARAMS
    uint32_t directory_paddr; // physical address for paging
} __attribute__((packed));

typedef struct page_directory page_directory_t;

void page_fault(int_regs_t*);

bool avail_frames(uint32_t count);

void set_frame(uint32_t addr);
void clear_frame(uint32_t addr);
bool test_frame(uint32_t addr);

uint32_t first_frame(void);
// uint32_t first_frame_from(uint32_t addr);

void alloc_frame(page_t* page, bool rw, bool user);
void free_frame(page_t* page);

void swap_dir(page_directory_t* dir);
void flush_tlb(void);

void reserve(uint32_t start, uint32_t length);

uint32_t v_to_paddr(uint32_t addr);

void copy_page_table_entries(page_table_t* src, page_table_t* dest);

void paging_init(multiboot_info_t* mbd, uint32_t magic);

page_t* get_page_from_vaddr(uint32_t vaddr, bool create, page_directory_t* dir);


#endif