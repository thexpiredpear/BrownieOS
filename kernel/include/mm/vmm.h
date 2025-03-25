#ifndef _KERNEL_VMM_H
#define _KERNEL_VMM_H 1

#include <stdint.h>
#include <mm/paging.h>

void* kmap(uint32_t paddr);
void kunmap(void* vaddr);

#endif