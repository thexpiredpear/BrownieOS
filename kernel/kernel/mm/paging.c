#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mm/paging.h>
#include <mm/kmm.h>
#include <core/multiboot.h>
#include <core/common.h>
#include <core/idt.h>
#include <core/isr.h>

uint64_t memory;
uint32_t* framemap;
uint32_t nframes;

extern uint32_t boot_page_directory[];
extern uint32_t boot_page_table[];

page_directory_t* kernel_directory;
page_directory_t* current_directory;

void page_fault(int_regs_t* registers) {
    printf("page fault!\n");
    uint32_t addr;
    asm volatile("mov %%cr2, %0" : "=r" (addr));
    bool present = registers->err_code & 0b1;
    bool write = registers->err_code & 0b10;
    bool user = registers->err_code & 0b100;
    bool reserved = registers->err_code & 0b1000;
    printf("addr: %x\n", addr);
    printf("present: %d\n", present);
    printf("write: %d\n", write);
    printf("user: %d\n", user);
    printf("reserved: %d\n", reserved);
    panic("page fault");
}

// set frame as used
void set_frame(uint32_t addr) {
    uint32_t frame = addr / 0x1000;
    uint32_t index = frame / 32;
    uint32_t offset = frame % 32;
    framemap[index] |= (0x1 << offset);
}

// clear a frame
void clear_frame(uint32_t addr) {
    uint32_t frame = addr / 0x1000;
    uint32_t index = frame / 32;
    uint32_t offset = frame % 32;
    framemap[index] &= ~(0x1 << offset);
}

// check if frame is set
bool test_frame(uint32_t addr) {
    uint32_t frame = addr / 0x1000;
    uint32_t index = frame / 32;
    uint32_t offset = frame % 32;
    return (framemap[index] & (0x1 << offset));
}

// return the first available frame
uint32_t first_frame() {
    for(uint32_t addr = 0; addr < memory; addr += 0x1000) {
        if(!test_frame(addr)) {
            return addr/0x1000;
        }
    }
    return -1;
}

// return first available frame starting from an address
/*
uint32_t first_frame_from(uint32_t addr) {
    addr &= 0xFFFFF000;
    for(addr; addr < memory; addr += 0x1000) {
        if(!test_frame(addr)) {
            return addr/0x1000;
        }
    }
    return -1;
}
*/

// check if there are count number of available frames
bool avail_frames(uint32_t count) {
    uint32_t free = 0;
    for(uint32_t frame = 0; frame < memory; frame += 0x1000) {
        if(!test_frame(frame)) {
            free++;
            if(free == count) {
                return true;
            }
        }
    }
    return false;
}

// TODO: function to allocate mult. frames at once sequentially for eff.

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
    page->present = 0;
    page->rw = 0;
    page->user = 0;
    page->accessed = 0;
    page->dirty = 0;
    page->unused = 0;
    page->frame = 0;
}

void swap_dir(page_directory_t* dir) {
    current_directory = dir;
    // Move the page directory address into the cr3 register
    asm volatile("mov %0, %%cr3":: "r"(dir->directory_paddr));
}

void flush_tlb() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0": "=r"(cr3));
    asm volatile("mov %0, %%cr3":: "r"(cr3));
}

void reserve(uint32_t start, uint32_t length) {
    uint32_t end;
    // round start to lower page boundary
    if(start % 0x1000 != 0) {
        start &= 0xFFFFF000;
    }
    // round length to upper page boundary
    if(length % 0x1000 != 0) {
        length = (length & 0xFFFFF000) + 0x1000;
    }
    length--;
    end = start + length;
    for(uint64_t addr = (uint64_t)start; addr < end; addr += 0x1000) {
        set_frame(addr);
    }
}

uint32_t v_to_paddr(uint32_t addr) {
    uint32_t table = addr / 0x400000;
    uint32_t page = (addr % 0x400000) / 0x1000;
    return (current_directory->tables[table]->pages[page].frame * 0x1000) + (addr % 0x1000);
}

void copy_page_table_entries(page_table_t* src, page_table_t* dest) {
    for(int i = 0; i < 1024; i++) {
        dest->pages[i] = src->pages[i];
    }
}

void paging_init(multiboot_info_t* mbd, uint32_t magic) {
    // Register page fault handler
    isr_set_handler(14, &page_fault);
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("Invalid multiboot magic number");
        return;
    }
    memory = 0x100000000; // TODO use grub memory map to determine memory size
    nframes = memory / 0x1000;
    // Bitmap of frames
    framemap = (uint32_t*)wmmalloc((nframes / 32)*sizeof(uint32_t));
    memset(framemap, 0, (nframes / 32)*sizeof(uint32_t));
    // Create kernel page directory
    kernel_directory = (page_directory_t*)wmmalloc_align(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));
    current_directory = kernel_directory;
    // Convert multiboot info physical address to virtual address
    mbd = (multiboot_info_t*)((uint32_t)mbd + 0xC0000000);
    if(!(mbd->flags & 0x00000020)) {
        panic("No memory map provided");
        return;
    }
    // Iterate through memory map & print info
    for(uint32_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)((uint32_t)((mbd->mmap_addr + i))+0xC0000000);
        // uint64_t addr = (uint64_t)(((uint64_t)(mmap->addr_high) << 32) | mmap->addr_low);
        // uint64_t len = (uint64_t)(((uint64_t)(mmap->len_high) << 32) | mmap->len_low);
        char* type;
        switch(mmap->type) {
            case 1:
                type = "Available";
                break;
            case 2:
                type = "Reserved";
                break;
            case 3:
                type = "ACPI Reclaimable";
                break;
            case 4:
                type = "NVS";
                break;
            case 5:
                type = "Bad Memory";
                break;
            default:
                type = "Unknown";
                break;
        }
        printf(
        "Base Address: 0x%x%x | Length: 0x%x%x | Type: %s\n", 
        mmap->addr_high, mmap->addr_low, mmap->len_high, mmap->len_low, type
        );
        if(mmap->type != 1) {
            reserve(mmap->addr_low, mmap->len_low);
        }
    }
    // Copy page tables from boot page directory
    for(int i = 0; i < 1024; i++) {
        uint32_t dir_entry = (uint32_t)boot_page_directory[i];
        if(dir_entry) {
            // virtual address to access page table
            page_table_t* boot_page_table = (page_table_t*)((dir_entry & 0xFFFFF000)+0xC0000000);
            page_table_t* new_page_table = (page_table_t*)wmmalloc_align(sizeof(page_table_t));
            for(int j = 0; j < 1024; j++) {
                new_page_table->pages[j] = boot_page_table->pages[j];
            }
            uint32_t new_page_table_paddr = (uint32_t)(new_page_table) - 0xC0000000;
            page_dir_entry_t new_dir_entry;
            new_dir_entry.present = 1;
            new_dir_entry.rw = 1;
            new_dir_entry.frame = new_page_table_paddr / 0x1000;
            kernel_directory->page_dir_entries[i] = new_dir_entry;
            kernel_directory->tables[i] = (page_table_t*)new_page_table;
        }
    }
    kernel_directory->directory_paddr = (uint32_t)(kernel_directory) - 0xC0000000;
    // Reserve first 4MiB of memory for system/kernel
    // TODO: only reserve first MiB automatically and base rest on page dir
    for(int addr = 0; addr < 0x400000; addr += 0x1000) {
        set_frame(addr);
    }
    swap_dir(kernel_directory);
}