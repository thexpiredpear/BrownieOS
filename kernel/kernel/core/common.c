#include <core/common.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <mm/kmm.h>
#include <mm/vmm.h>

// TODO: implement w/ kmalloc
ordered_array_t init_ordered_array(uint32_t max_size) {
   ordered_array_t ordered_array;
   ordered_array.array = (uint32_t*)kmalloc(max_size * sizeof(uint32_t));
   ordered_array.size = 0;
   ordered_array.max_size = max_size;
   ordered_array.predicate = &less_predicate;
   memset(ordered_array.array, 0, max_size * sizeof(uint32_t));
   return ordered_array;
}

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
   return NULL;
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

void destroy_ordered_array(ordered_array_t* ordered_array) {
   kfree(ordered_array->array);
}

void print_with_leading_zeros(uint32_t len, char* str) {
   uint32_t str_len = strlen(str);
   for(uint32_t i = 0; i < len - str_len; i++) {
      printf("0");
   }
   printf(str);
   printf("\n");
}

bool less_predicate(uint32_t a, uint32_t b) {
   return a < b ? true : false;
}

void trigger_interrupt(uint8_t i) {
   // asm volatile("int %0" : : "i"(i));
   i++;
}

void trigger_page_fault() {
   uint32_t fault = *((uint32_t*)0x0);
   fault++;
}

void outb(uint16_t port, uint8_t value) {
   asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port) {
   uint8_t ret;
   asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
   return ret;
}

uint16_t inw(uint16_t port) {
   uint16_t ret;
   asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
   return ret;
}  

void cli() {
   asm volatile("cli");
}

void sti() {
   asm volatile("sti");
}

void get_msr(uint32_t msr, uint32_t* lo, uint32_t* hi) {
   asm volatile("rdmsr":"=a"(*lo),"=d"(*hi):"c"(msr));
}

void set_msr(uint32_t msr, uint32_t lo, uint32_t hi) {
   asm volatile("wrmsr"::"a"(lo),"d"(hi),"c"(msr));
}

__attribute__((noreturn)) 
void panic(char* message) {
   printf("kernel panic: ");
   printf(message);
   asm volatile("cli");
   asm volatile("hlt");
}

void gdb_stop() {
   printf("gdb stop\n");
}