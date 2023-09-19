#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <kernel/common.h>
#include <kernel/mm/kheap.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/vmm.h>

extern uint32_t _kernel_end;
extern page_directory_t* kernel_directory;
extern page_directory_t* current_directory;
uint32_t kheap = (uint32_t)&_kernel_end;

ordered_array_t header_array_info;
ordered_array_t footer_array_info;
heap_info_t info;
header_t* header_ptr_array[1024];
footer_t* footer_ptr_array[1024];

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
    header_array_info = init_ordered_array_place((void*)(&header_ptr_array), 0x400);
    footer_array_info = init_ordered_array_place((void*)(&footer_ptr_array), 0x400);
    void* start = kalloc_pages(1024);
    void* end = start + (1024 * 0x1000);
    header_t* init_header = (header_t*)start;
    footer_t* init_footer = (footer_t*)(end - sizeof(footer_t));
    start += sizeof(header_t);
    end -= sizeof(footer_t);
    info.start = (uint32_t)start;
    info.end = (uint32_t)end;
    info.size = info.end - info.start;
    info.free = info.size;
    info.used = 0;
    info.magic = KHEAP_MAGIC_64;
    insert_ordered_array(&header_array_info, (uint32_t)init_header);
    insert_ordered_array(&footer_array_info, (uint32_t)init_footer);
}
