#ifndef _KERNEL_KHEAP_H
#define _KERNEL_KHEAP_H 1

#define KHEAP_MAGIC_64 0xB10CB10CB10CB10C
#define KHEAP_MAGIC_32 0xB10CB10C

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

struct heap_info { 
    uint32_t start; // start of heap
    uint32_t end; // end of heap
    uint32_t max; // max size of heap
    uint32_t size; // current size of heap
    uint32_t free; // free space in heap
    uint32_t used; // used space in heap
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
};

// 16 bytes
struct footer {
    header_t* header; // pointer to associated header
    header_t* next; // pointer to next header
    uint64_t magic; // 0xB10CB10CB10CB10C   
};

uint32_t wmmalloc(size_t size);
uint32_t wmmalloc_align(size_t size);

#endif