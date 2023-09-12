#ifndef _KERNEL_KHEAP_H
#define _KERNEL_KHEAP_H 1

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

uint32_t wmmalloc(size_t size);
uint32_t wmmalloc_align(size_t size);

#endif