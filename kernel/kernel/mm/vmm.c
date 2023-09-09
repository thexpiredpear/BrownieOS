#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/paging.h>

extern page_directory_t* current_directory;

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
    // if physical pages allocated, return start ptr
    // start at 0xC0000000 (3GiB) for kernel space mapping
    // FIXME: only works for <= 1024 (cant allocate new page tables!)
    uint32_t contig = 0;
    // uint32_t table_changes = 0;
    void* start = 0;
    void* end = 0;
    // TODO: make this more efficient
    // track for new page tables needed 

    if(!avail_frame(pages)) {
        // TODO: panic? not really sure
        return 0;
    }

    for(int i = 768; i < 1024; i++) {
        if(current_directory->tables[i]) {
            for(int j = 0; j < 1024; j++) {
                if(current_directory->tables[i]->pages[j].frame) {
                    contig++;
                    if(contig == 1) {
                        start = (void*)(i * 0x400000 + j * 0x1000);
                    }
                    if(contig == pages) {
                        // Allocate physical pages
                        end = (void*)(i * 0x400000 + j * 0x1000);
                        // int table_start = start / 0x400000;
                        // int page_start = (start % 0x400000) / 0x1000;
                        void* pos = start;
                        int cur_table = 0;
                        int cur_page = 0;
                        while(pos <= end) {
                            cur_table = (uint32_t)(pos / 0x400000);
                            cur_page = (uint32_t)((pos % 0x400000) / 0x1000);
                            if(!current_directory->tables[cur_table]->pages[cur_page].frame) {
                                alloc_frame(
                                &current_directory->
                                tables[cur_table]->
                                pages[cur_page],
                                false, true);
                            }
                            pos += 0x1000;
                        }
                        // increment pos until reaching ending address
                        // use / 0x400000 and % 0x400000 / 0x1000 to get indices
                        return start;
                    }
                } else {
                    table_changes = 0;
                    contig = 0;
                    start = 0;
                }
            }
        } else {
            table_changes = 0;
            contig = 0;
            start = 0;
        }
        if(contig) {
            table_changes++;
        }
    }
}