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

void print_kheap() {
    // printf("heap info - start: %x, end: %x, magic: %x\n", info.start, info.end, info.magic);

    for(int i = 0; i < header_array_info.size; i++) {
        header_t* header = (header_t*)get_ordered_array(&header_array_info, i);
        footer_t* footer = (footer_t*)get_ordered_array(&footer_array_info, i);
        uint32_t block_start = ((uint32_t)header + 0x00000010) & 0xFF;
        uint32_t footer_end = ((uint32_t)footer + 0x00000010) & 0xFF;
        char* used = (header->used == 0) ? "free" : "used";
        printf("%x|%x --%s-- %x|%x\n", (uint32_t)header, block_start, used, (uint32_t)footer, footer_end);
    }
}

void* kmalloc(size_t size) {
    if(size == 0) {
        return 0xFFFFFFFF;
    }
    if(!kmm_prechecks(&info, &header_array_info, &footer_array_info)) {
        return 0xFFFFFFFF;
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
        if(!kmm_checks(header, footer)) {
            return 0xFFFFFFFF;
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

void kfree(void* ptr) {
    header_t* header = (header_t*)((uint32_t)ptr - sizeof(header_t));
    footer_t* footer = header->footer;
    if(!kmm_prechecks(&info, &header_array_info, &footer_array_info)) {
        return;
    }
    if(!kmm_checks(header, footer)) {
        return;
    }
    unify(header, footer);
    return;
}

void unify(header_t* header, footer_t* footer) {
    header = unify_left(header, footer);
    unify_right(header, footer);
    header->used = 0;
    return;
}

header_t* unify_left(header_t* header, footer_t* footer) {
    uint32_t index = find_ordered_array(&header_array_info, (uint32_t)header);
    header_t* ret = header;
    if(index > 0) {
        header_t* prev_header = (header_t*)get_ordered_array(&header_array_info, index - 1);
        footer_t* prev_footer = prev_header->footer;
        if(prev_header->used == 0) {
            printf("coalescing left from %x to %x\n", (uint32_t)prev_header, (uint32_t)footer + sizeof(footer_t));
            prev_header->size += header->size + sizeof(header_t) + sizeof(footer_t);
            prev_header->footer = footer;
            footer->header = prev_header;
            header->size = 0;
            header->footer = 0;
            header->used = 0;
            header->magic = 0;
            prev_footer->header = 0;
            prev_footer->res = 0;
            prev_footer->magic = 0;
            remove_ordered_array(&footer_array_info, index - 1);
            remove_ordered_array(&header_array_info, index);
            ret = prev_header;
        }
    }
    return ret;
}

footer_t* unify_right(header_t* header, footer_t* footer) {
    uint32_t index = find_ordered_array(&header_array_info, (uint32_t)header);
    footer_t* ret = footer;
    if(index < header_array_info.size - 1) {
        header_t* next_header = (header_t*)get_ordered_array(&header_array_info, index + 1);
        footer_t* next_footer = next_header->footer;
        if(next_header->used == 0) {
            printf("coalescing right from %x to %x\n", (uint32_t)header, (uint32_t)next_footer + sizeof(footer_t));
            header->size += next_header->size + sizeof(header_t) + sizeof(footer_t);
            header->footer = next_footer;
            next_footer->header = header;
            footer->header = 0;
            footer->res = 0;
            footer->magic = 0;
            next_header->size = 0;
            next_header->footer = 0;
            next_header->used = 0;
            next_header->magic = 0;
            remove_ordered_array(&footer_array_info, index);
            remove_ordered_array(&header_array_info, index + 1);
            ret = next_footer;
        }
    }
    return ret;
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
    header->footer = new_footer;
    header->used = 1;

    insert_ordered_array(&header_array_info, (uint32_t)new_header);
    insert_ordered_array(&footer_array_info, (uint32_t)new_footer);

    return header;
}

bool kmm_prechecks(heap_info_t* heap_info, 
ordered_array_t* header_arr_info, ordered_array_t* footer_arr_info) {
    bool ret = true;
    if(heap_info->magic != KHEAP_MAGIC_64) {
        printf("HEAP MAGIC FAILED\n");
        ret = false;
    }
    if(header_arr_info->size != footer_arr_info->size) {
        printf("HEADER AND FOOTER ARRAY SIZE MISMATCH\n");
        ret = false;
    }
    return ret;
}

bool kmm_checks(header_t* header, footer_t* footer) {
    bool ret = true;
    if(header->magic != KHEAP_MAGIC_32) {
        printf("HEADER MAGIC FAILED %x\n", header->magic);
        ret = false;
    }
    if(footer->magic != KHEAP_MAGIC_64) {
        printf("FOOTER MAGIC FAILED %x\n", footer->magic);
        ret = false;
    }
    if(header->footer != footer) {
        printf("HEADER DOES NOT POINT TO FOOTER %x\n", header->footer);
        ret = false;
    }
    if(footer->header != header) {
        printf("FOOTER DOES NOT POINT TO HEADER %x\n", footer->header);
        ret = false;
    }
    return ret;
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