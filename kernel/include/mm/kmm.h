#ifndef _KERNEL_KMM_H
#define _KERNEL_KMM_H 1

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <core/common.h>
#include <mm/paging.h>

#define KHEAP_MAGIC_64 0xB10CB10CB10CB10C
#define KHEAP_MAGIC_32 0xB10CB10C

#define KHEAP_PAGES (1024)

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

// Dumps the current kernel heap layout: prints each block’s header/footer
// virtual addresses and allocation state. Reads the global `kheap` metadata
// and associated ordered arrays; does not modify heap state.
void print_kheap();

// Intended internal helper: allocate from an existing free block described by
// `header`/`footer`, splitting the block if sufficient space remains. Returns
// the header of the newly allocated block. Pointer parameters are KERNEL
// virtual addresses to heap metadata structures.
header_t* alloc_from_header(header_t* header, footer_t* footer, size_t size);
// Allocates from the free block `header`/`footer` within `heap`. If the block
// is larger than `size` + metadata, it is split and the remainder is inserted
// back into the ordered arrays. Returns the header of the allocated block.
header_t* alloc_from_block(heap_t* heap, header_t* header, footer_t* footer, size_t size);

// Allocates `size` bytes from `heap`, 16-byte aligning payload addresses.
// Scans the ordered arrays for a suitable free block, validates header/footer
// integrity, and splits as needed. Returns a KERNEL virtual pointer to the
// payload (immediately after the block header), or NULL on failure.
void* alloc(heap_t* heap, size_t size);
// Frees a previously allocated payload pointer from `heap`. Computes its block
// header/footer (virtual), validates integrity, coalesces with adjacent free
// blocks via `unify_*`, and updates the ordered arrays. No physical pages are
// released here beyond block metadata updates.
void free(heap_t* heap, void* ptr);

// Convenience wrapper over `alloc` using the global kernel heap `kheap`.
// Returns a KERNEL virtual pointer to usable memory, 16-byte aligned.
void* kmalloc(size_t size);
// Frees a pointer previously returned by `kmalloc` back to `kheap`.
void kfree(void* ptr);

// Coalesces an entire free block with any adjacent free neighbors. Internally
// calls `unify_left` then `unify_right` to merge blocks and mark the result
// free. Pointer parameters are KERNEL virtual addresses.
void unify(heap_t* heap, header_t* header, footer_t* footer);
// If the previous (lower-address) block is free, merges it with the current
// block, updates metadata and ordered arrays, and returns the resulting header.
header_t* unify_left(heap_t* heap, header_t* header, footer_t* footer);
// If the next (higher-address) block is free, merges it with the current block,
// updates metadata and ordered arrays, and returns the resulting footer.
footer_t* unify_right(heap_t* heap, header_t* header, footer_t* footer);

// Sanity checks prior to heap operations: validates `heap` magic and that the
// header/footer ordered arrays are consistent in size. Does not modify state.
bool kmm_prechecks(heap_t* heap, 
ordered_array_t* header_array, ordered_array_t* footer_array);
// Validates an individual block’s integrity: checks header/footer magic values
// and that `header->footer` and `footer->header` form a consistent pair.
bool kmm_checks(header_t* header, footer_t* footer);

// Initializes the global kernel heap `kheap`: allocates an initial region of
// physical pages via `alloc_pages`, maps it into the kernel window, seeds a
// single large free block (header/footer), and initializes the ordered arrays.
void kheap_init();

#endif
