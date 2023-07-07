#ifndef _KERNEL_COMMON_H
#define _KERNEL_COMMON_H 1

#include <stdint.h>

void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);

#endif