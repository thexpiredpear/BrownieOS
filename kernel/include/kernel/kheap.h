#ifndef _KERNEL_KHEAP_H
#define _KERNEL_KHEAP_H 1

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

void kheap_init(void);
uint32_t kmalloc(size_t size);
uint32_t kmalloc_align(size_t size);

#endif