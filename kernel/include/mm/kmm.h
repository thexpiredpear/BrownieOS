#ifndef _KERNEL_KMM_H
#define _KERNEL_KMM_H 1

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <core/common.h>
#include <mm/paging.h>

#define KHEAP_MAGIC_64 0xB10CB10CB10CB10C
#define KHEAP_MAGIC_32 0xB10CB10C
#define PHYS_MAPPING_SPACE 0x400000
#define KERN_END 0xFFFFFFFF
#define PAGE_STRUCT_END (KERN_END - PHYS_MAPPING_SPACE)
// 0xFFBFFFFF
#define PHYS_MAPPING_START (PAGE_STRUCT_END + 1)
// 0xFFC00000
#define KHEAP_END (PAGE_STRUCT_END - (65 * 1024 * PAGE_TABLE_SIZE))
// 0xEF7FFFFF
#define PAGE_STRUCT_START (KHEAP_END + 1)
// 0xEF800000
// 64*4MiB for process page tables, 1*4MiB for process page dirs
// | 0x0 | 0xC0000000 | 0xC0400000 | 0xEF800000 | 0xFFC00000 | EOM
// | USR | KPROG      | KHEAP      | PAGESTRUCT | PHYSMAP    | EOM
// | 3GB | 4MB        | 756MB      | 260MB      | 4MB        | 4GB      

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