#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <kernel/kheap.h>

extern uint32_t _kernel_end;
extern uint32_t boot_page_directory;
uint32_t kheap = (uint32_t)&_kernel_end;

uint32_t kmalloc(size_t size) {
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