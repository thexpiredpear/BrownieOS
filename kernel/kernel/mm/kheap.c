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
uint32_t wmloc = (uint32_t)&_kernel_end;

ordered_array_t header_array_info;
ordered_array_t footer_array_info;
heap_info_t info;
header_t* header_ptr_array[1024];
footer_t* footer_ptr_array[1024];

uint32_t wmmalloc(size_t size) {
    if(wmloc & 0x00000007) {
        wmloc &= 0xFFFFFFF8;
        wmloc += 0x00000008;
    }
    uint32_t ret = wmloc;
    wmloc += size;
    return ret;
}

uint32_t wmmalloc_align(size_t size) {
    if(wmloc & 0x00000FFF) {
        wmloc &= 0xFFFFF000;
        wmloc += 0x00001000;
    }
    uint32_t ret = wmloc;
    wmloc += size;
    return ret;
}

void* kmalloc(size_t size) {
    if(size == 0) {
        return NULL;
    }
    if(!kmalloc_prechecks(&info, &header_array_info, &footer_array_info)) {
        return NULL;
    }
    if(size % 16 != 0) {
        size += (16 - (size % 16));
    }
    for(uint32_t i = 0; i < header_array_info.size && i < footer_array_info.size; i++) {
        header_t* header = (header_t*)get_ordered_array(&header_array_info, i);
        footer_t* footer = (footer_t*)get_ordered_array(&footer_array_info, i);
        if(header->used != 0 || header->size < size) {
            continue;
        }
        if(!kmalloc_checks(header, footer)) {
            return NULL;
        }
        // suitable block found; header & footer valid
        // if block able to split into new block > than 16 bytes free, split
        if(header->size >= size + sizeof(header_t) + sizeof(footer_t) + 16) {
            return (void*)((uint32_t)alloc_from_header(header, footer, size) + sizeof(header_t));
        } else {
            // use whole block
            header->used = 1;
            footer->res = 0;
            return (void*)((uint32_t)header + sizeof(header_t));
        }
    }
    // no suitable block found; allocate new block
    uint32_t bytes = size + sizeof(header_t) + sizeof(footer_t);
    uint32_t pages = bytes / 0x1000;
    if(bytes % 0x1000 != 0) {
        pages++;
    }
    uint32_t start = (uint32_t)kalloc_pages(pages);
    uint32_t end = start + (pages * 0x1000);
    info.end = end;
    header_t* header = (header_t*)start;
    footer_t* footer = (footer_t*)(end - sizeof(footer_t));
    header->size = end - (start + sizeof(header_t) + sizeof(footer_t));
    header->footer = footer;
    header->used = 0;
    header->magic = KHEAP_MAGIC_32;
    
    footer->header = header;
    footer->res = 0;
    footer->magic = KHEAP_MAGIC_64;

    return (void*)((uint32_t)alloc_from_header(header, footer, size) + sizeof(header_t));
}

header_t* alloc_from_header(header_t* header, footer_t* footer, size_t size) {
    footer_t* new_footer = (footer_t*)((uint32_t)header + size + sizeof(header_t)); // place new footer @ end of new block
    header_t* new_header = (header_t*)((uint32_t)new_footer + sizeof(footer_t)); // place new header @ start of new block

    new_footer->header = header;
    new_footer->magic = KHEAP_MAGIC_64;
    new_footer->res = 0;

    new_header->size = header->size - (size + sizeof(header_t) + sizeof(footer_t)); 
    new_header->footer = footer;
    new_header->used = 0;
    new_header->magic = KHEAP_MAGIC_32;

    footer->header = new_header;

    header->size = size;
    header->used = 1;

    insert_ordered_array(&header_array_info, (uint32_t)new_header);
    insert_ordered_array(&footer_array_info, (uint32_t)new_footer);

    return header;
}

bool kmalloc_prechecks(heap_info_t* heap_info, 
ordered_array_t* header_arr_info, ordered_array_t* footer_arr_info) {
    if(heap_info->magic != KHEAP_MAGIC_64) {
        printf("HEAP MAGIC FAILED\n");
        return false;
    }
    if(header_arr_info->size != footer_arr_info->size) {
        printf("HEADER AND FOOTER ARRAY SIZE MISMATCH\n");
        return false;
    }
    return true;
}

bool kmalloc_checks(header_t* header, footer_t* footer) {
    if(header->magic != KHEAP_MAGIC_32) {
        printf("HEADER MAGIC FAILED\n");
        return false;
    }
    if(footer->magic != KHEAP_MAGIC_64) {
        printf("FOOTER MAGIC FAILED\n");
        return false;
    }
    if(header->footer != footer) {
        printf("HEADER DOES NOT POINT TO FOOTER\n");
        return false;
    }
    if(footer->header != header) {
        printf("FOOTER DOES NOT POINT TO HEADER\n");
        return false;
    }
    return true;
}

void kheap_init() {
    header_array_info = init_ordered_array_place((void*)(&header_ptr_array), 0x400);
    footer_array_info = init_ordered_array_place((void*)(&footer_ptr_array), 0x400);
    uint32_t start = (uint32_t)kalloc_pages(1024);
    uint32_t end = start + (1024 * 0x1000);
    header_t* init_header = (header_t*)start;
    footer_t* init_footer = (footer_t*)(end - sizeof(footer_t));
    info.start = (uint32_t)start;
    info.end = (uint32_t)end;
    info.magic = KHEAP_MAGIC_64;

    init_header->magic = KHEAP_MAGIC_32;
    init_header->size = end - (start + sizeof(header_t) + sizeof(footer_t));
    init_header->used = 0;
    init_header->footer = init_footer;

    init_footer->magic = KHEAP_MAGIC_64;
    init_footer->header = init_header;
    init_footer->res = 0;

    insert_ordered_array(&header_array_info, (uint32_t)init_header);
    insert_ordered_array(&footer_array_info, (uint32_t)init_footer);
}