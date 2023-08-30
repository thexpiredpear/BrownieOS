#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H 1

#include <stdint.h>
#include <stdbool.h>
#include <kernel/multiboot.h>

typedef struct page {
    uint32_t present    : 1;   // Present in memory if set
    uint32_t rw         : 1;   // Readwrite if set
    uint32_t user       : 1;   // User mode if set
    uint32_t accessed   : 1;   // Has the page been accessed since last refresh?
    uint32_t dirty      : 1;   // Has the page been written to since last refresh?
    uint32_t unused     : 7;   // Unused / reserved bits
    uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table {
    page_t pages[1024];
} page_table_t;

typedef struct page_directory {
    uint32_t tables_paddr[1024]; // physical addresses for paging
    page_table_t* tables[1024]; // virtual addresses for r/w access
    uint32_t directory_paddr; // physical address for paging
} page_directory_t;

void page_fault(regs_t*)

void set_frame(uint32_t addr);
void clear_frame(uint32_t addr);
bool test_frame(uint32_t addr);

uint32_t first_frame(void);

void alloc_frame(page_t* page, bool rw, bool user);
void free_frame(page_t* page);

void swap_dir(page_directory_t* dir);
void flush_tlb(void);

void paging_init(multiboot_info_t* mbd, uint32_t magic);

page_t* get_page_from_vaddr(uint32_t vaddr, bool create, page_directory_t* dir);


#endif