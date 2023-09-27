#include <kernel/common.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

// TODO: implement w/ kmalloc
// ordered_array_t init_ordered_array(uint32_t max_size) {}

ordered_array_t init_ordered_array_place(void* addr, uint32_t max_size) {
   ordered_array_t ordered_array;
   ordered_array.array = (uint32_t*)addr;
   ordered_array.size = 0;
   ordered_array.max_size = max_size;
   ordered_array.predicate = &less_predicate;
   memset(ordered_array.array, 0, max_size * sizeof(uint32_t));
   return ordered_array;
}

uint32_t insert_ordered_array(ordered_array_t* ordered_array, uint32_t val) {
   if(ordered_array->size < ordered_array->max_size) {
         uint32_t i = ordered_array->size;
         while(i > 0 && ordered_array->predicate(val, ordered_array->array[i - 1])) { 
            ordered_array->array[i] = ordered_array->array[i - 1];
            i--;
         }
         ordered_array->array[i] = val;
         ordered_array->size++;
         return i;
   }
   return 0xFFFFFFFF;
}

void remove_ordered_array(ordered_array_t* ordered_array, uint32_t i) {
   while(i < ordered_array->size - 1) {
      ordered_array->array[i] = ordered_array->array[i + 1]; 
      i++;
   }
   ordered_array->array[i] = 0; // zero out last element, out of bounds
   ordered_array->size--;
}

uint32_t get_ordered_array(ordered_array_t* ordered_array, uint32_t i) {
   return ordered_array->array[i];
}

uint32_t find_ordered_array(ordered_array_t* ordered_array, uint32_t val) {
   uint32_t i = 0;
   while(i < ordered_array->size) {
      if(ordered_array->array[i] == val) {
         return i;
      }
      i++;
   }
   return 0xFFFFFFFF;
}

// TODO: implement w/ kfree
// void destroy_ordered_array(ordered_array_t* ordered_array) {}

bool less_predicate(uint32_t a, uint32_t b) {
   return a < b ? true : false;
}

void outb(uint16_t port, uint8_t value)
{
   asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port)
{
   uint8_t ret;
   asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
   return ret;
}

uint16_t inw(uint16_t port)
{
   uint16_t ret;
   asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
   return ret;
}  

__attribute__((noreturn)) 
void panic(const char* message)
{
   printf("kernel panic: ");
   printf(message);
   asm volatile("cli");
   asm volatile("hlt");
}