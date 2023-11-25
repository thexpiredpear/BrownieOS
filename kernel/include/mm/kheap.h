#ifndef _KERNEL_KHEAP_H
#define _KERNEL_KHEAP_H 1

#define KHEAP_MAGIC_64 0xB10CB10CB10CB10C
#define KHEAP_MAGIC_32 0xB10CB10C

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <core/common.h>

struct heap { 
    uint16_t user;
    uint16_t rw;
    uint32_t start; // start of heap
    uint32_t end; // end of heap
    uint32_t max_addr; // max address of heap
    ordered_array_t* header_array;
    ordered_array_t* footer_array;
    uint64_t magic; // 0xB10CB10CB10CB10C
};

typedef struct heap heap_t;

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

void print_kheap();

header_t* alloc_from_header(header_t* header, footer_t* footer, size_t size);
header_t* alloc_from_block(heap_t* heap, header_t* header, footer_t* footer, size_t size);

void* alloc(heap_t* heap, size_t size);
void free(heap_t* heap, void* ptr);

void* kmalloc(size_t size);
void kfree(void* ptr);

void unify(heap_t* heap, header_t* header, footer_t* footer);
header_t* unify_left(heap_t* heap, header_t* header, footer_t* footer);
footer_t* unify_right(heap_t* heap, header_t* header, footer_t* footer);

bool kmm_prechecks(heap_t* heap, 
ordered_array_t* header_array, ordered_array_t* footer_array);
bool kmm_checks(header_t* header, footer_t* footer);

void kheap_init();

#endif