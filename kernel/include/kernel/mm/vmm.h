#ifndef _KERNEL_VMM_H
#define _KERNEL_VMM_H 1

void* alloc_pages(size_t pages);
void free_pages(void* vaddr, size_t pages);

#endif