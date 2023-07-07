#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <kernel/paging.h>

uint32_t framemap[];
uint32_t nframes = 2**32/0x1000 // TODO use grub memory map to determine memory size

