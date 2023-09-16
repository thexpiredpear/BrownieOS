#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <kernel/mm/kheap.h>
#include <kernel/mm/paging.h>

extern uint32_t _kernel_end;
extern page_directory_t* kernel_directory;
extern page_directory_t* current_directory;
uint32_t kheap = (uint32_t)&_kernel_end;
ordered_array_t* header_array;
ordered_array_t* footer_array;
kheap_info_t info;
header_t* header_array_ptr[0x400];
footer_t* footer_array_ptr[0x400];

uint32_t wmmalloc(size_t size) {
    if(kheap & 0x00000007) {
        kheap &= 0xFFFFFFF8;
        kheap += 0x00000008;
    }
    uint32_t ret = kheap;
    kheap += size;
    return ret;
}

uint32_t wmmalloc_align(size_t size) {
    if(kheap & 0x00000FFF) {
        kheap &= 0xFFFFF000;
        kheap += 0x00001000;
    }
    uint32_t ret = kheap;
    kheap += size;
    return ret;
}

void kheap_init() {
    header_array = init_ordered_array_place((void*)(&header_array_ptr), 0x400);
    footer_array = init_ordered_array_place((void*)(&footer_array_ptr), 0x400);
    info.start = (uint32_t)kalloc_pages(1024);
    info.end = info.start + 0x400000;
    info.size = info.end - info.start;
    info.max = (uint32_t)(0x100000000 - info.start)
    info.free = info.size;
    info.used = 0;
    info.magic = KHEAP_MAGIC;
}
