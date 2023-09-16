#ifndef _KERNEL_COMMON_H
#define _KERNEL_COMMON_H 1

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
// ordered array impl, order on insertion
struct ordered_array {
    uint32_t* array;
    uint32_t size;
    uint32_t max_size;  
    predicate_t predicate;
}

typedef bool (*predicate_t)(uint32_t, uint32_t);

typedef struct ordered_array ordered_array_t;

// ordered array functions

// ordered_array_t init_ordered_array(uint32_t max_size);
ordered_array_t init_ordered_array_place(uint32_t* addr, uint32_t max_size);
bool insert_ordered_array(uint32_t val, ordered_array_t* array);
void remove_ordered_array(uint32_t index, ordered_array_t* array);
uint32_t get_ordered_array(uint32_t index, ordered_array_t* array);
uint32_t find_ordered_array(uint32_t val, ordered_array_t* array);
// void destroy_ordered_array(ordered_array_t* array);

void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
void panic();

#endif