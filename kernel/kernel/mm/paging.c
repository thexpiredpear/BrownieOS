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

page_directory_t kernel_directory_aligned;
page_directory_t* kernel_directory;
page_directory_t* current_directory;

page_table_t kernel_page_table[1024 - KERN_START_PAGE_TBL];

void page_fault(int_regs_t* registers) {
    printf("page fault!\n");
    uint32_t addr;
    asm volatile("mov %%cr2, %0" : "=r" (addr));
    char* present = (registers->err_code & PAGE_FAULT_PRESENT_A) ? "true" : "false";
    char* write = (registers->err_code & PAGE_FAULT_WRITE_A) ? "true" : "false";
    char* user = (registers->err_code & PAGE_FAULT_USER_A) ? "true" : "false";
    char* reserved = (registers->err_code & PAGE_FAULT_RESERVED_A) ? "true" : "false";
    printf("addr: %x\n", addr);
    printf("present: %s\n", present);   
    printf("write: %s\n", write);
    printf("user: %s\n", user);
    printf("reserved: %s\n", reserved);
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
    for(uint32_t addr = 0; addr < memory; addr += PAGE_SIZE) {
        if(!test_frame(addr)) {
            return PAGE_FRAME(addr);
        }
    }
    return -1;
}

// check if there are count number of available frames
bool avail_frames(uint32_t count) {
    uint32_t free = 0;
    for(uint32_t frame = 0; frame < memory; frame += PAGE_SIZE) {
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
    *((uint32_t*)(page)) = 0;
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

void set_pde(page_dir_entry_t* pde, uint8_t present, uint8_t rw, uint8_t user, uint32_t frame) {
    pde->present = present;
    pde->rw = rw;
    pde->user = user;
    pde->frame = frame;
}

void set_page(page_t* page, uint8_t present, uint8_t rw, uint8_t user, uint32_t frame) {
    page->present = present;
    page->rw = rw;
    page->user = user;
    page->frame = frame;
}

void setup_kernel_directory() {
    kernel_directory->directory_paddr = KV2P((uint32_t)kernel_directory);
    page_table_t* cur_table;
    // identity map all pages in lowmem (896MiB)
    for(int i = KERN_START_PAGE_TBL; i < KERN_HIGHMEM_START_PAGE_TBL; i++) {
        cur_table = &kernel_page_table[i - KERN_START_PAGE_TBL];
        kernel_directory->tables[i] = cur_table;
        set_pde(&(kernel_directory->page_dir_entries[i]), 1, 1, 0, 
            // frame of pde is first 20 bits of physical table addr (last 12 bit zeroes)
            PAGE_FRAME(KV2P((uint32_t)cur_table)));
        for(int j = 0; j < 1024; j++) {
            set_page(&(cur_table->pages[j]), 1, 1, 0, 
                // set page frame as current virtual address minus kernel offset / 0x1000
                PAGE_FRAME(KV2P((PAGE_IDX_VADDR(i, j, 0)))));
        }
    }
    // 128MiB reserved for specific phys mappings, devices, large contiguous buffers
    for(int i = KERN_HIGHMEM_START_PAGE_TBL; i < 1024; i++) {
        cur_table = &kernel_page_table[i - KERN_START_PAGE_TBL];
        kernel_directory->tables[i] = cur_table;
        set_pde(&(kernel_directory->page_dir_entries[i]), 1, 1, 0,
            PAGE_FRAME(KV2P((uint32_t)cur_table)));
    }
    // reserve first GiB of pages in framemap
    for(int addr = 0; addr < 1024 * 1024 * 1024; addr += PAGE_SIZE) {
        set_frame(addr);
    }
}

void paging_init(multiboot_info_t* mbd, uint32_t magic) {
    // Register page fault handler
    isr_set_handler(14, &page_fault);
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("Invalid multiboot magic number");
        return;
    }
    memory = EOM; // TODO use grub memory map to determine memory size
    nframes = PAGE_FRAME(memory) + 1;
    // Bitmap of frames
    framemap = (uint32_t*)wmmalloc((nframes / 32) * sizeof(uint32_t));
    memset(framemap, 0, (nframes / 32) * sizeof(uint32_t));
    // Create kernel page directory
    kernel_directory = &kernel_directory_aligned;
    memset(kernel_directory, 0, sizeof(page_directory_t));
    // Copy page tables from boot page directory
    setup_kernel_directory();
    swap_dir(kernel_directory);
    current_directory = kernel_directory;  
    terminal_initialize();
    reserve_mem_map(mbd);
}       