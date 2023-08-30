#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <kernel/kheap.h>
#include <kernel/paging.h>

extern uint32_t _kernel_end;
extern page_directory_t* kernel_directory;
uint32_t kheap = (uint32_t)&_kernel_end;

uint32_t kmalloc(size_t size) {
    if(kheap & 0x00000007) {
        kheap &= 0xFFFFFFF8;
        kheap += 0x00000008;
    }
    uint32_t ret = kheap;
    kheap += size;
    return ret;
}

uint32_t kmalloc_align(size_t size) {
    if(kheap & 0x00000FFF) {
        kheap &= 0xFFFFF000;
        kheap += 0x00001000;
    }
    uint32_t ret = kheap;
    kheap += size;
    return ret;
}
/*
uint32_t kmalloc_phys(size_t size, uint32_t* phys) {
    if(kheap & 0x00000007) {
        kheap &= 0xFFFFFFF8;
        kheap += 0x00000008;
    }
    uint32_t ret = kheap;
    uint32_t dir_idx = ret >> 22;
    uint32_t table_idx = ret >> 12 & 0x03FF;
    if(kernel_directory->tables[dir_idx]) {
        if(kernel_directory->tables[dir_idx]->pages[table_idx]) {
            *phys = kernel_directory->tables[dir_idx]->pages[table_idx]->frame * 0x1000 + (ret & 0x00000FFF);
        }
    }
}
*/