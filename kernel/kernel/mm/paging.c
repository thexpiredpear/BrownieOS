#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <kernel/paging.h>
#include <kernel/kheap.h>
#include <kernel/multiboot.h>
#include <kernel/common.h>
#include <kernel/idt.h>
#include <kernel/isr.h>

uint64_t memory;
uint32_t* framemap;
uint32_t nframes;

extern uint32_t boot_page_directory[];
extern uint32_t boot_page_table[];

page_directory_t* kernel_directory;
page_directory_t* current_directory;

void page_fault(regs_t* registers) {
    printf("page fault!\n");
    uint32_t addr;
    asm volatile("mov %%cr2, %0" : "=r" (addr));
    bool present = registers->err_code & 0x1;
    bool write = registers->err_code & 0x2;
    bool user = registers->err_code & 0x4;
    bool reserved = registers->err_code & 0x8;
    printf("addr: %x\n", addr);
    printf("present: %d\n", present);
    printf("write: %d\n", write);
    printf("user: %d\n", user);
    printf("reserved: %d\n", reserved);
    return;
}

void set_frame(uint32_t addr) {
    uint32_t frame = addr / 0x1000;
    uint32_t index = frame / 32;
    uint32_t offset = frame % 32;
    framemap[index] |= (0x1 << offset);
}

void clear_frame(uint32_t addr) {
    uint32_t frame = addr / 0x1000;
    uint32_t index = frame / 32;
    uint32_t offset = frame % 32;
    framemap[index] &= ~(0x1 << offset);
}

bool test_frame(uint32_t addr) {
    uint32_t frame = addr / 0x1000;
    uint32_t index = frame / 32;
    uint32_t offset = frame % 32;
    return (framemap[index] & (0x1 << offset));
}

uint32_t first_frame() {
    for(uint32_t frame = 0; frame < memory; frame += 0x1000) {
        if(!test_frame(frame)) {
            return frame/0x1000;
        }
    }
    return -1;
}

void alloc_frame(page_t* page, bool user, bool rw) {
    if(page->frame != 0) {
        return;
    }
    uint32_t frame = first_frame();
    set_frame(frame * 0x1000);
    page->present = 1;
    page->rw = (rw) ? 1 : 0;
    page->user = (user) ? 1 : 0;
    page->frame = frame;
}

void free_frame(page_t* page) {
    uint32_t frame = page->frame;
    if(!frame) {
        return;
    }
    clear_frame(frame * 0x1000);
    page->frame = 0;
}

void swap_dir(page_directory_t* dir) {
    current_directory = dir;
    // Move the page directory address into the cr3 register
    asm volatile("mov %0, %%cr3":: "r"(&dir->tables_paddr));
}

void flush_tlb() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0": "=r"(cr3));
    asm volatile("mov %0, %%cr3":: "r"(cr3));
}

void paging_init(multiboot_info_t* mbd, uint32_t magic) {
    isr_set_handler(14, &page_fault);
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("Invalid multiboot magic number");
        return;
    }
    mbd = (multiboot_info_t*)((uint32_t)mbd + 0xC0000000);
    if(!(mbd->flags & 0x00000020)) {
        panic("No memory map provided");
        return;
    }
    for(int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)((uint32_t)((mbd->mmap_addr + i))+0xC0000000);
        char* type = (mmap->type == 1) ? "Available" : "Reserved";
        printf("Base Address: %x%x | Length: %x%x | Type: %s\n", mmap->addr_high, mmap->addr_low, mmap->len_high, mmap->len_low, type);
    }   
    memory = (uint64_t)0x100000000; // TODO use grub memory map to determine memory size
    nframes = memory / 0x1000;
    // Bitmap of frames
    framemap = (uint32_t*)kmalloc((nframes / 32)*sizeof(uint32_t));
    memset(framemap, 0, (nframes / 32)*sizeof(uint32_t));
    // Create kernel page directory
    kernel_directory = (page_directory_t*)kmalloc_align(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));
    current_directory = kernel_directory;
    // Copy page tables from boot page directory
    for(int i = 0; i < 1024; i++) {
        kernel_directory->tables_paddr[i] = (uint32_t)boot_page_directory[i];
        kernel_directory->tables[i] = (page_table_t*)((uint32_t)boot_page_directory[i] + 0xC0000000);
    }
    kernel_directory->directory_paddr = (uint32_t)(&kernel_directory) - 0xC0000000;
    // Reserve first 1MB of memory
    for(int tmp = 0; tmp < 0x100000; tmp += 0x1000) {
        set_frame(tmp);
    }
    // TODO Reserve rest of unavailable memory from grub memory map
}