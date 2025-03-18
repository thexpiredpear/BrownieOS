#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mm/paging.h>
#include <mm/kmm.h>
#include <drivers/tty.h>
#include <core/multiboot.h>
#include <core/common.h>
#include <core/idt.h>
#include <core/isr.h>

uint8_t framemap[NFRAMES];


extern uint32_t boot_page_directory[];
extern uint32_t boot_page_table[];

page_directory_t kernel_directory_aligned;
page_directory_t* kernel_directory;
page_directory_t* current_directory;
page_table_t kernel_page_tables[1024 - KERN_START_TBL];

void page_fault(int_regs_t* registers) {
    printf("page fault!\n");
    uint32_t addr;
    asm volatile("mov %%cr2, %0" : "=r" (addr));
    bool present = registers->err_code & PAGE_FAULT_PRESENT_A;
    bool write = registers->err_code & PAGE_FAULT_WRITE_A;
    bool user = registers->err_code & PAGE_FAULT_USER_A;
    bool reserved = registers->err_code & PAGE_FAULT_RESERVED_A;
    printf("addr: %x\n", addr);
    printf("present: %d\n", present);
    printf("write: %d\n", write);
    printf("user: %d\n", user);
    printf("reserved: %d\n", reserved);
    panic("page fault");
} __attribute__((noreturn));

// set frame as used
void set_frame(uint32_t addr) {
    uint32_t frame = PAGE_FRAME(addr);
    uint32_t index = PAGE_FRAME_BITMAP_IDX(frame);
    uint32_t offset = PAGE_FRAME_BITMAP_OFF(frame);
    framemap[index] |= (0x1 << offset);
}

// clear a frame
void clear_frame(uint32_t addr) {
    uint32_t frame = PAGE_FRAME(addr);
    uint32_t index = PAGE_FRAME_BITMAP_IDX(frame);
    uint32_t offset = PAGE_FRAME_BITMAP_OFF(frame);
    framemap[index] &= ~(0x1 << offset);
}

// check if frame is set
bool test_frame(uint32_t addr) {
    uint32_t frame = PAGE_FRAME(addr);
    uint32_t index = PAGE_FRAME_BITMAP_IDX(frame);
    uint32_t offset = PAGE_FRAME_BITMAP_OFF(frame);
    return (framemap[index] & (0x1 << offset));
}

// return the first available frame
uint32_t first_frame() {
    for(uint32_t addr = 0; addr < EOM; addr += PAGE_SIZE) {
        if(!test_frame(addr)) {
            return PAGE_FRAME(addr);
        }
    }
    return -1;
}

// check if there are count number of available frames
bool avail_frames(uint32_t count) {
    uint32_t free = 0;
    for(uint32_t frame = 0; frame < EOM; frame += PAGE_SIZE) {
        if(!test_frame(frame)) {
            free++;
            if(free == count) {
                return true;
            }
        }
    }
    return false;
}

void alloc_frame(page_t* page, bool user, bool rw) {
    if(page->frame != 0) {
        return;
    }
    uint32_t frame = first_frame();
    set_frame(frame * PAGE_SIZE);
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
    clear_frame(frame * PAGE_SIZE);
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
    start = PAGE_ROUND_DOWN(start);
    // round length to upper page boundary
    length = PAGE_ROUND_UP(length);
    end = start + length - 1;
    for(uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
        set_frame(addr);
    }
}

uint32_t v_to_paddr(uint32_t addr) {
    uint32_t table = PAGE_DIR_IDX(addr);
    uint32_t page = PAGE_TBL_IDX(addr);
    return (current_directory->tables[table]->pages[page].frame * PAGE_SIZE) + (addr % PAGE_SIZE);
}

void copy_page_table_entries(page_table_t* src, page_table_t* dest) {
    for(int i = 0; i < 1024; i++) {
        dest->pages[i] = src->pages[i];
    }
}

void reserve_mem_map(multiboot_info_t* mbd) {
    mbd = (multiboot_info_t*)((uint32_t)mbd + 0xC0000000);
    if(!(mbd->flags & 0x00000020)) {
        panic("No memory map provided");
        return;
    }
    for(uint32_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)((uint32_t)((mbd->mmap_addr + i))+0xC0000000);        // uint64_t len = (uint64_t)(((uint64_t)(mmap->len_high) << 32) | mmap->len_low);
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
}

void cp_boot_dir(uint32_t start_tbl, uint32_t end_tbl) {
    for(int i = start_tbl; i <= end_tbl; i++) {
        uint32_t dir_entry = (uint32_t)boot_page_directory[i];
        page_table_t* new_page_table = (page_table_t*)wmmalloc_align(sizeof(page_table_t));
        if(dir_entry) {
            page_table_t* boot_page_table = (page_table_t*)((dir_entry & 0xFFFFF000)+0xC0000000);
            for(int j = 0; j < 1024; j++) {
                new_page_table->pages[j] = boot_page_table->pages[j];
            }
        }
        uint32_t new_page_table_paddr = (uint32_t)(new_page_table) - 0xC0000000;
        page_dir_entry_t new_dir_entry;
        new_dir_entry.present = 1;
        new_dir_entry.rw = 1;
        new_dir_entry.frame = new_page_table_paddr / PAGE_SIZE;
        kernel_directory->page_dir_entries[i] = new_dir_entry;
        kernel_directory->tables[i] = (page_table_t*)new_page_table;
    }
}

void setup_kernel_directory() {
    kernel_directory->directory_paddr = (uint32_t)(kernel_directory) - 0xC0000000;
    page_table_t* cur_table;
    for(uint32_t i = KERN_DMA_START_TBL; i < 1024; i++) {
        cur_table = &kernel_page_tables[i - KERN_START_TBL];
        kernel_directory->tables[i] = cur_table;
        uint32_t cur_table_paddr = (uint32_t)(cur_table) - 0xC0000000;
        kernel_directory->page_dir_entries[i].frame = (uint32_t)(cur_table_paddr) / PAGE_SIZE;
        kernel_directory->page_dir_entries[i].present = 1;
        //if(i >= KERN_NORMAL_START_TBL) {
            kernel_directory->page_dir_entries[i].rw = 1;
        //}
        for(int j = 0; j < 1024; j++) {
            // only set frame if should be id mapped (between 0MiB & 896MiB)
            // highmem reserved for phys mapping, large virtually contig buffers, etc...
            if(i < KERN_HIGHMEM_START_TBL) {
                cur_table->pages[j].frame = (i - KERN_START_TBL) * 1024 + j;
            }
            cur_table->pages[j].present = 1;
            cur_table->pages[j].rw = 1;
        }
    }
}

void paging_init(multiboot_info_t* mbd, uint32_t magic) {
    // Register page fault handler
    isr_set_handler(14, &page_fault);
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("Invalid multiboot magic number");
        return;
    }
    // Create kernel page directory
    kernel_directory = &kernel_directory_aligned;
    setup_kernel_directory();
    kernel_directory->directory_paddr = (uint32_t)(kernel_directory) - 0xC0000000;
    swap_dir(kernel_directory);
    current_directory = kernel_directory;
    terminal_initialize();
    reserve_mem_map(mbd);
}