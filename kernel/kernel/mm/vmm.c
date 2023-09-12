#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/paging.h>
#include <kernel/kheap.h>

extern page_directory_t* kernel_directory;

void* kalloc_pages(size_t pages) {
    // ONLY USED FOR KERNEL MAPPING
    // Find contiguous virtual address space in current directory
    // Iterate through page tables
    // Alg:
    // create a start ptr and contig page count
    // iterate through page dir once
    // skip over null page tables and reset contig & start ptr
    // iterate through pages in current table
    // if page is free, contig++ & if contig == 0, set start ptr
    // if page is not free, reset contig page count and start ptr
    // if contig page count == pages, try to allocate physical pages
    // copy entries from tables[] to dir_entry[]
    // if physical pages allocated, return start ptr
    // start at 0xC0000000 (3GiB) for kernel space mapping
    // TODO: only works for <= 1024 (cant allocate new page tables!)
    uint32_t needed_null_tables = (pages - 1) / 1024 + 1;
    uint32_t contig = 0;
    uint32_t contig_null_tables = 0;
    void* start = 0;
    uint32_t start_null_tables = 0;
    bool found = false;
    // TODO: make this more efficient
    // track for new page tables needed 

    if(!avail_frames(pages)) {
        // TODO: panic? not really sure
        return 0;
    }
    // start in kernel space
    for(int i = 768; i < 1024; i++) {
        page_table_t* table = kernel_directory->tables[i];
        if((uint32_t)table != 0) {
            contig_null_tables = 0;
            for(int j = 0; j < 1024; j++) {
                page_t page = table->pages[j];
                // some monstrosity to bitmask the page
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