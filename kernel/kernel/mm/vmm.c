#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/kheap.h>

extern page_directory_t* kernel_directory;
extern page_directory_t* current_directory;

void* kalloc_pages(size_t pages) {
    // ONLY USED FOR KERNEL MAPPING
    uint32_t needed_null_tables = (pages - 1) / 1024 + 1;
    uint32_t contig = 0;
    uint32_t contig_null_tables = 0;
    void* start = 0;
    uint32_t start_null_tables = 0;
    bool found = false;
    if(!avail_frames(pages)) {
        return (void*)0xFFFFFFFF;
    }
    // start in kernel space
    for(int i = 768; i < 1024; i++) {
        page_table_t* table = kernel_directory->tables[i];
        if((uint32_t)table != 0) {
            contig_null_tables = 0;
            for(int j = 0; j < 1024; j++) {
                page_t page = table->pages[j];
                // monstrosity to bitmask page, access as uint32_t
                if(*(uint32_t*)&page == 0) {
                    contig++;
                    if(contig == 1) {
                        start = (void*)(i * 0x400000 + j * 0x1000);
                    }
                    if(contig == pages) {
                        // Allocate physical pages
                        void* pos = start;
                        int cur_table = 0;
                        int cur_page = 0;
                        while(contig) {
                            cur_table = (uint32_t)((uint32_t)pos / 0x400000);
                            cur_page = (uint32_t)(((uint32_t)pos % 0x400000) / 0x1000);
                            if(!kernel_directory->tables[cur_table]->pages[cur_page].frame) {
                                alloc_frame(
                                &kernel_directory->
                                tables[cur_table]->
                                pages[cur_page],
                                false, true);
                            }
                            pos += 0x1000;
                            contig--;
                        }
                        // increment pos until reaching ending address
                        // use / 0x400000 and % 0x400000 / 0x1000 to get indices
                        return start;
                    }
                } else {
                    contig = 0;
                }
            }
        } else if (!found) {
            contig_null_tables++;
            if(contig_null_tables == 1) {
                start_null_tables = i;
            }       
            if (contig_null_tables == needed_null_tables) {
                found = true;
            }    
            contig = 0;
        }
    }
    if(contig_null_tables == needed_null_tables) {
        // Allocate new page tables
        while(contig_null_tables) {
            uint32_t index = start_null_tables + (needed_null_tables - contig_null_tables);
            page_table_t* table = (page_table_t*)wmmalloc_align(sizeof(page_table_t));
            memset(table, 0, sizeof(page_table_t));
            kernel_directory->tables[index] = table;
            uint32_t phys = v_to_paddr((uint32_t)table);
            page_dir_entry_t new_dir_entry;
            new_dir_entry.present = 1;
            new_dir_entry.rw = 1;
            new_dir_entry.frame = phys / 0x1000;
            kernel_directory->page_dir_entries[index] = new_dir_entry;
            contig_null_tables--;
        }
        // Allocate physical pages
        contig = pages;
        void* pos = (void*)(start_null_tables * 0x400000);
        int cur_table = start_null_tables;
        int cur_page = 0;
        while(contig) {
            cur_table = (uint32_t)((uint32_t)pos / 0x400000);
            cur_page = (uint32_t)(((uint32_t)pos % 0x400000) / 0x1000);
            if(!kernel_directory->tables[cur_table]->pages[cur_page].frame) {
                alloc_frame(
                &kernel_directory->
                tables[cur_table]->
                pages[cur_page],
                false, true);
            }
            pos += 0x1000;
            contig--;
        }
        return (void*)(start_null_tables * 0x400000);
    }
    return (void*)0xFFFFFFFF;
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