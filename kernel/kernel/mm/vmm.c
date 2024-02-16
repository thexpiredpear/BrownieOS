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
    for(uint32_t i = 768; i < 1023; i++) {
        page_table_t* table = kernel_directory->tables[i];
        for(uint32_t j = 0; j < 1024; j++) {
            page_t page = table->pages[j];
            if(*(uint32_t*)&page == 0) {
                if(++contig == 1) {
                    start = (void*)PAGE_IDX_VADDR(i, j, 0);
                } 
                if(contig == pages) {
                    break;
                }   
            } else {
                contig = 0;
                start = NULL;
            }
        }
        if(contig == pages) {
            break;
        }
    }
    if((start == NULL) || (contig != pages)) {
        panic("not enough free kernel space");
    }
    void* pos = start;
    int cur_table = 0;
    int cur_page = 0;
    while(contig) {
        cur_table = (uint32_t)(PAGE_DIR_IDX(pos));
        cur_page = (uint32_t)(PAGE_TBL_IDX(pos));
        alloc_frame(
            &(kernel_directory->tables[cur_table]->pages[cur_page]),
            false, true);
        pos += PAGE_SIZE;
        contig--;
    }
    return start;
}

void free_pages(void* addr, size_t pages) {
    // TODO: free page tables if fully empty
    uint32_t start = (uint32_t)addr;
    uint32_t end = start + (pages * PAGE_SIZE);
    uint32_t table;
    uint32_t page;
    for(uint32_t pos = start; pos < end; pos += PAGE_SIZE) {
        table = PAGE_DIR_IDX(pos);
        page = PAGE_TBL_IDX(pos);
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
            page->frame = paddr / PAGE_SIZE;
            return (void*)(0xFFC00000 + (i * PAGE_SIZE) + (paddr % PAGE_SIZE));
        }
    }
    return NULL;
}

void clraccess_paddr_DANGER(void* vaddr) {
    page_table_t* table = kernel_directory->tables[PAGE_DIR_IDX(vaddr)];
    page_t* page = &(table->pages[PAGE_TBL_IDX(vaddr)]);
    *(uint32_t*)page = 0;
    return;
}