#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <mm/paging.h>
#include <core/common.h>

uint32_t pid_ctr = 0;
proc_t* current_proc;

void clone_page_dir(page_directory_t* src, page_directory_t* dest) {
    for(int i = 0; i < 1024, i++) {
        // copy entries for kernel pages, should refer to same physical addresses
        if(i < KERN_START_TBL) {
            dest->page_dir_entries[i] = src->page_dir_entries[i];
            dest->tables[i] = src->tables[i];
        } else if(src->page_dir_entries.frame != 0) {
            // copy the content of all mapped pages into new pages in the new directory
            // TODO: implement as a copy-on-write in conjunction w/ page fault handler
            // allocate a new page table
            page_table_t* dest_page_table = KP2V(alloc_pages(PMM_FLAGS_DEFAULT, 1)); 
            dest->tables[i] = dest_page_table;
            // copy all data except page frame & metadata
            dest->page_dir_entries[i] = src->page_dir_entries[i]
            dest->page_dir_entries[i].frame = KV2P(dest_page_table) / PAGE_SIZE;
            dest->page_dir_entries[i].accessed = 0;
            // loop thru pages
            for(int j = 0; j < 1024; j++) {
                // if page mapped
                if(src->tables[i]->pages[j].frame != 0) {
                    // then get the sources virtual address, allocate a new HIGHMEM page and 
                    // temporarily map to lowmem for access
                    void* src_page_vaddr = PAGE_IDX_VADDR(i, j, 0);
                    uint32_t paddr = alloc_pages(PMM_FLAGS_HIGHMEM, 1);
                    void* tmp_dest_page_vaddr = kmap(paddr);
                    memcpy(tmp_dest_page_vaddr, src_page_vaddr, PAGE_SIZE);
                    kunmap(tmp_dest_page_vaddr);
                    // copy all data except meta data & set frame
                    dest->tables[i]->pages[j] = src->tables[i]->pages[j];
                    dest->tables[i]->pages[j].accessed = 0;
                    dest->tables[i]->pages[j].dirty = 0;
                    dest->tables[i]->pages[j].frame = paddr / PAGE_SIZE;
                }
            }
        }
    }
}

void proc_init() {
    
}