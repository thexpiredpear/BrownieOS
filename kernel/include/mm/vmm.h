#ifndef _KERNEL_VMM_H
#define _KERNEL_VMM_H 1

void* kalloc_pages(size_t pages);
void free_pages(void* addr, size_t pages);

void* access_paddr_DANGER(uint32_t paddr);
void clraccess_paddr_DANGER(void* vaddr);

#endif