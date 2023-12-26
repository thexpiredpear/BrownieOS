#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mm/vmm.h>
#include <mm/paging.h>
#include <mm/kmm.h>

extern page_directory_t* kernel_directory;
extern page_directory_t* current_directory;

void* kalloc_pages(size_t pages) {
    // ONLY USED FOR KERNEL MAPPING
    uint32_t contig = 0;
    void* start = 0;
    if(!avail_frames(pages)) {
        return NULL;
    }
    // start in kernel space
    for(int i = 768; i < 1023; i++) {
        page_table_t* table = kernel_directory->tables[i];
        if((uint32_t)table != 0) {
            for(int j = 0; j < 1024; j++) {
                page_t page = table->pages[j];
                // monstrosity to bitmask page, access as uint32_t
                if(*(uint32_t*)&page == 0) {
                    if(++contig == 1) {
                        start = (void*)(i * 0x400000 + j * 0x1000);
                    } 
                    if(contig == pages) {
                        break;
                    }   
                } else {
                    contig = 0;
                    start = NULL;
                }
            }
        } else {
            contig += 1024;
            if(start == NULL) {
                start = (void*)(i * 0x400000);
            }
            if(contig >= pages) {
                contig = pages;
                break;
            }
        }
    }
    if(start == NULL) {
        panic("no free kernel space");
    }
    void* pos = start;
    int cur_table = 0;
    int cur_page = 0;
    while(contig) {
        cur_table = (uint32_t)((uint32_t)pos / 0x400000);
        cur_page = (uint32_t)(((uint32_t)pos % 0x400000) / 0x1000);
        if((uint32_t)kernel_directory->tables[cur_table] != 0) {
            // page table exists
            if(!kernel_directory->tables[cur_table]->pages[cur_page].frame) {
                alloc_frame(
                &(kernel_directory->tables[cur_table]->pages[cur_page]),
                false, true);
            }
            pos += 0x1000;
            contig--;
        } else {
            // page table does not exist
            page_table_t* table = (page_table_t*)wmmalloc_align(sizeof(page_table_t));
            memset(table, 0, sizeof(page_table_t));
            kernel_directory->tables[cur_table] = table;
            uint32_t phys = v_to_paddr((uint32_t)table);
            page_dir_entry_t new_dir_entry;
            new_dir_entry.present = 1;
            new_dir_entry.rw = 1;
            new_dir_entry.frame = phys / 0x1000;
            kernel_directory->page_dir_entries[cur_table] = new_dir_entry;
        }
    }
    return start;
}

void free_pages(void* addr, size_t pages) {
    // TODO: free page tables if fully empty
    uint32_t start = (uint32_t)addr;
    uint32_t end = start + (pages * 0x1000);
    uint32_t table;
    uint32_t page;
    for(uint32_t pos = start; pos < end; pos += 0x1000) {
        table = pos / 0x400000;
        page = (pos % 0x400000) / 0x1000;
        free_frame(&current_directory->tables[table]->pages[page]);
    }
}

void* access_paddr_DANGER(uint32_t paddr) {
    // find a free kernel space page
    page_table_t* table = kernel_directory->tables[1023];
    for(int i = 0; i < 1024; i++) {
        page_t* page = &(table->pages[i]);
        if(*(uint32_t*)page == 0) {
            page->present = 1;
            page->rw = 1;
            page->user = 0;
            page->frame = paddr / 0x1000;
            return (void*)(0xFFC00000 + (i * 0x1000) + (paddr % 0x1000));
        }
    }
    return NULL;
}

void clraccess_paddr_DANGER(void* vaddr) {
    page_table_t* table = kernel_directory->tables[(uint32_t)vaddr / 0x400000];
    page_t* page = &(table->pages[((uint32_t)vaddr % 0x400000) / 0x1000]);
    *(uint32_t*)page = 0;
    return;
}