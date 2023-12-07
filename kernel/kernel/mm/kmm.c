#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <core/common.h>
#include <mm/kmm.h>
#include <mm/paging.h>
#include <mm/vmm.h>

extern uint32_t _kernel_end;
extern page_directory_t* kernel_directory;
extern page_directory_t* current_directory;
uint32_t wmloc = (uint32_t)&_kernel_end;

ordered_array_t kheap_header_array;
ordered_array_t kheap_footer_array;
heap_t kheap;
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

bool kmm_prechecks(heap_t* heap, ordered_array_t* header_array, ordered_array_t* footer_array) {
    bool ret = true;
    if(heap->magic != KHEAP_MAGIC_64) {
        printf("HEAP MAGIC FAILED\n");
        ret = false;
    }
    if(header_array->size != footer_array->size) {
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

void print_kheap() {
    printf("heap info - start: %x, end: %x, magic: %x\n", kheap.start, kheap.end, kheap.magic);
    for(uint32_t i = 0; i < kheap_header_array.size; i++) {
        header_t* header = (header_t*)get_ordered_array(&kheap_header_array, i);
        footer_t* footer = (footer_t*)get_ordered_array(&kheap_footer_array, i);
        uint32_t block_start = ((uint32_t)header + 0x00000010) & 0xFF;
        uint32_t footer_end = ((uint32_t)footer + 0x00000010) & 0xFF;
        char* used = (header->used == 0) ? "free" : "used";
        printf("%x|%x --%s-- %x|%x\n", (uint32_t)header, block_start, used, (uint32_t)footer, footer_end);
    }
}

header_t* alloc_from_block(heap_t* heap, header_t* header, footer_t* footer, size_t size) {
    // check for at least 64 free space to split
    if(size >= (header->size - sizeof(header_t) - sizeof(footer_t) - 64)) {
        header->used = 1;
        return header;
    } else { // split block
        // place place new footer and header in middle of block to split
        // new footer at start of new block + size
        footer_t* new_footer = (footer_t*)((uint32_t)header + size + sizeof(header_t)); 
        // new header after new footer
        header_t* new_header = (header_t*)((uint32_t)new_footer + sizeof(footer_t));

        // footer of new block points to old header
        new_footer->header = header;
        new_footer->magic = KHEAP_MAGIC_64;
        new_footer->res = 0;

        // new header size = old size - (size + header + footer)
        new_header->size = header->size - (size + sizeof(header_t) + sizeof(footer_t)); 
        new_header->footer = footer;
        new_header->used = 0;
        new_header->magic = KHEAP_MAGIC_32;

        // footer of old block points to new header
        footer->header = new_header;

        header->size = size;
        header->footer = new_footer;
        header->used = 1;

        // add header and footer to arrays
        insert_ordered_array(heap->header_array, (uint32_t)new_header);
        insert_ordered_array(heap->footer_array, (uint32_t)new_footer);

        return header;
    }
}

header_t* unify_left(heap_t* heap, header_t* header, footer_t* footer) {
    ordered_array_t* header_array = heap->header_array;
    ordered_array_t* footer_array = heap->footer_array;
    // find index of header & footer in arrays
    uint32_t index = find_ordered_array(header_array, (uint32_t)header);
    header_t* ret = header;
    // if block to left exists
    if(index > 0) {
        // get header & footer of block to left
        header_t* prev_header = (header_t*)get_ordered_array(header_array, index - 1);
        footer_t* prev_footer = prev_header->footer;
        // if block to left free coalesce 
        if(prev_header->used == 0) {
            printf("coalescing left from %x to %x\n", (uint32_t)prev_header, (uint32_t)footer + sizeof(footer_t));
            // previous header size = size of whole coalesced block
            prev_header->size += header->size + sizeof(header_t) + sizeof(footer_t);
            // previous header footer = footer of current block
            prev_header->footer = footer;
            // point footer back to left header
            footer->header = prev_header;

            // clear current header
            header->size = 0;
            header->footer = 0;
            header->used = 0;
            header->magic = 0;

            // clear prev footer
            prev_footer->header = 0;
            prev_footer->res = 0;
            prev_footer->magic = 0;
            // remove prev footer and cur header from arrays
            remove_ordered_array(footer_array, index - 1);
            remove_ordered_array(header_array, index);
            ret = prev_header;
        }
    }
    return ret;
}

footer_t* unify_right(heap_t* heap, header_t* header, footer_t* footer) {
    ordered_array_t* header_array = heap->header_array;
    ordered_array_t* footer_array = heap->footer_array;
    // find index of header & footer in arrays
    uint32_t index = find_ordered_array(header_array, (uint32_t)header);
    footer_t* ret = footer;
    // if block to right exists
    if(index < header_array->size - 1) {
        // get header & footer of block to right
        header_t* next_header = (header_t*)get_ordered_array(header_array, index + 1);
        footer_t* next_footer = next_header->footer;
        // if block to right free coalesce
        if(next_header->used == 0) {
            printf("coalescing right from %x to %x\n", (uint32_t)header, (uint32_t)next_footer + sizeof(footer_t));
            // current header size = size of whole coalesced block
            header->size += next_header->size + sizeof(header_t) + sizeof(footer_t);
            // current header footer = footer of next block
            header->footer = next_footer;
            // point next footer back to current header
            next_footer->header = header;

            // clear footer
            footer->header = 0;
            footer->res = 0;
            footer->magic = 0;

            // clear next header
            next_header->size = 0;
            next_header->footer = 0;
            next_header->used = 0;
            next_header->magic = 0;
            // remove footer and next header from arrays
            remove_ordered_array(footer_array, index);
            remove_ordered_array(header_array, index + 1);
            ret = next_footer;
        }
    }
    return ret;
}

void unify(heap_t* heap, header_t* header, footer_t* footer) {
    header = unify_left(heap, header, footer);
    unify_right(heap, header, footer);
    header->used = 0;
    return;
}

void* alloc(heap_t* heap, size_t size) {
    ordered_array_t* header_array = heap->header_array;
    ordered_array_t* footer_array = heap->footer_array;
    if(size == 0) {
        return NULL;
    }
    if(!kmm_prechecks(heap, header_array, footer_array)) {
        return NULL;
    }
    // align on 16 byte boundary
    if(size % 16 != 0) {
        size += (16 - (size % 16));
    }
    // search through blocks
    for(uint32_t i = 0; i < header_array->size; i++) {
        header_t* header = (header_t*)get_ordered_array(header_array, i);
        footer_t* footer = (footer_t*)get_ordered_array(footer_array, i);
        if(!kmm_checks(header, footer)) {
            // corrupted, fix 
            continue;
        }
        // if block used or too small, continue
        if(header->used != 0 || header->size < size) {
            continue;
        }
        // suitable block found; header & footer valid
        if(size <= header-> size) {
            // alloc
            return (void*)((uint32_t)alloc_from_block(heap, header, footer, size) + sizeof(header_t));
        } 
    }
    
    // no suitable block found; allocate new block via vmm
    uint32_t bytes = size + sizeof(header_t) + sizeof(footer_t);
    uint32_t pages = bytes / 0x1000;
    // round to higher page
    if(bytes % 0x1000 != 0) {
        pages++;
    }
    // check if available heap space
    if(heap->end + (pages * 0x1000) > heap->max_addr) {
        printf("heap full\n");
        return NULL;
    }
    // allocate pages
    uint32_t start = (uint32_t)kalloc_pages(pages);
    uint32_t end = start + (pages * 0x1000);
    heap->end = end;
    // create header and footer for new block
    header_t* header = (header_t*)start;
    footer_t* footer = (footer_t*)(end - sizeof(footer_t));
    // size of new block = end - (start + header + footer)
    header->size = end - (start + sizeof(header_t) + sizeof(footer_t));
    header->footer = footer;
    header->used = 0;
    header->magic = KHEAP_MAGIC_32;

    footer->header = header;
    footer->res = 0;
    footer->magic = KHEAP_MAGIC_64;

    // add header and footer to arrays
    insert_ordered_array(header_array, (uint32_t)header);
    insert_ordered_array(footer_array, (uint32_t)footer);
    // alloc
    return (void*)((uint32_t)alloc_from_block(heap, header, footer, size) + sizeof(header_t));
}

void free(heap_t* heap, void* ptr) {
    header_t* header = (header_t*)((uint32_t)ptr - sizeof(header_t));
    footer_t* footer = header->footer;
    if(!kmm_prechecks(heap, heap->header_array, heap->footer_array)) {
        return;
    }
    if(!kmm_checks(header, footer)) {
        return;
    }
    unify(heap, header, footer);
    return;
}

void* kmalloc(size_t size) {
    return alloc(&kheap, size);
}

void kfree(void* ptr) {
    free(&kheap, ptr);
}

void kheap_init() {
    kheap_header_array = init_ordered_array_place((void*)(&header_ptr_array), 0x400);
    kheap_footer_array = init_ordered_array_place((void*)(&footer_ptr_array), 0x400);
    uint32_t start = (uint32_t)kalloc_pages(1024);
    uint32_t end = start + (1024 * 0x1000);
    header_t* init_header = (header_t*)start;
    footer_t* init_footer = (footer_t*)(end - sizeof(footer_t));
    kheap.user = 0;
    kheap.rw = 1;
    kheap.start = (uint32_t)start;
    printf("kheap start: %x\n", kheap.start);
    kheap.end = (uint32_t)end;
    kheap.max_addr = 0xFBFFFFFF;
    kheap.header_array = &kheap_header_array;
    kheap.footer_array = &kheap_footer_array;
    kheap.magic = KHEAP_MAGIC_64; 

    init_header->magic = KHEAP_MAGIC_32;
    init_header->size = end - (start + sizeof(header_t) + sizeof(footer_t));
    init_header->used = 0;
    init_header->footer = init_footer;

    init_footer->magic = KHEAP_MAGIC_64;
    init_footer->header = init_header;
    init_footer->res = 0;

    insert_ordered_array(&kheap_header_array, (uint32_t)init_header);
    insert_ordered_array(&kheap_footer_array, (uint32_t)init_footer);

    // last page table reserved for physical access
    page_table_t* table = (page_table_t*)wmmalloc_align(sizeof(page_table_t));
    memset(table, 0, sizeof(page_table_t));
    kernel_directory->tables[1023] = table;
    uint32_t phys = v_to_paddr((uint32_t)table);
    page_dir_entry_t new_dir_entry;
    new_dir_entry.present = 1;
    new_dir_entry.rw = 1;
    new_dir_entry.frame = phys / 0x1000;
    kernel_directory->page_dir_entries[1023] = new_dir_entry;
}