#ifndef _KERNEL_KHEAP_H
#define _KERNEL_KHEAP_H 1

#define KHEAP_MAGIC_64 0xB10CB10CB10CB10C
#define KHEAP_MAGIC_32 0xB10CB10C

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <kernel/common.h>

struct heap_info { 
    uint32_t start; // start of heap
    uint32_t end; // end of heap
    uint64_t magic; // 0xB10CB10CB10CB10C
};

typedef struct heap_info heap_info_t;

typedef struct footer footer_t;
typedef struct header header_t;
// forward decls

// 16 bytes
struct header {
    uint32_t size; // size of block
    footer_t* footer; // pointer to associated footer
    uint32_t used; // 0 if free, 1 if used
    uint32_t magic; // 0xB10CB10C
} __attribute__((packed));

// 16 bytes
struct footer {
    header_t* header; // pointer to associated header
    uint32_t res; // reserved bits
    uint64_t magic; // 0xB10CB10CB10CB10C   
} __attribute__((packed));

uint32_t wmmalloc(size_t size);
uint32_t wmmalloc_align(size_t size);

void* kmalloc(size_t size);

header_t* alloc_from_header(header_t* header, footer_t* footer, size_t size);

bool kmalloc_prechecks(heap_info_t* heap_info, 
ordered_array_t* header_arr_info, ordered_array_t* footer_arr_info);
bool kmalloc_checks(header_t* header, footer_t* footer);

void kheap_init();

#endif