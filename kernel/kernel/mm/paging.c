#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mm/paging.h>
#include <mm/kmm.h>
#include <mm/vmm.h>
#include <drivers/tty.h>
#include <core/multiboot.h>
#include <core/common.h>
#include <core/idt.h>
#include <core/isr.h>

uint8_t framemap[NFRAMES];

page_directory_t kernel_directory_aligned;
page_directory_t* kernel_directory;
page_table_t kernel_page_tables[1024 - KERN_START_TBL];

void page_fault(int_regs_t* registers) {
    printf("page fault!\n");
    uint32_t addr;
    asm volatile("mov %%cr2, %0" : "=r" (addr));
    bool protection_violation = registers->err_code & PAGE_FAULT_PRESENT_A;
    bool write = registers->err_code & PAGE_FAULT_WRITE_A;
    bool user = registers->err_code & PAGE_FAULT_USER_A;
    bool reserved = registers->err_code & PAGE_FAULT_RESERVED_A;
    bool instruction = registers->err_code & PAGE_FAULT_INSTR_FETCH_A;
    printf("addr: %x\n", addr);
    printf("page-protection violation (1=protection,0=non-present): %d\n", protection_violation);
    printf("caused by write access: %d\n", write);
    printf("originated from user mode: %d\n", user);
    printf("reserved-bit set in entry: %d\n", reserved);
    printf("instruction fetch: %d\n", instruction);
    printf("eip: %x cs: %x\n", registers->eip, registers->cs);
    printf("esp: %x useresp: %x ss: %x\n", registers->esp, registers->useresp, registers->ss);
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


uint32_t alloc_pages(pmm_flags_t flags, uint32_t count) {
    // Physical address range split:
    // - Lowmem (identity-mapped by the kernel): [0, identity_phys_end)
    // - Highmem (requires temporary mapping via kmap): [identity_phys_end, EOM]
    const uint32_t identity_phys_end = KERN_IDENTITY_PHYS_END; // 896 MiB
    uint32_t paddr = 0;
    uint32_t end = EOM;
    if(flags & PMM_FLAGS_HIGHMEM) {
        // Start allocating at the first page beyond the identity-mapped region
        paddr = identity_phys_end;
    } else {
        // Constrain search to identity-mapped low memory
        end = identity_phys_end;
    }
    for(uint32_t contig = 0; paddr < end; paddr += PAGE_SIZE) {
        if(!test_frame(paddr)) {
            if(++contig == count) {
                // Found a contiguous range; mark frames as used before returning
                uint32_t base = paddr - PAGE_PADDR((count - 1));
                for(uint32_t i = 0; i < count; i++) {
                    set_frame(base + i * PAGE_SIZE);
                }
                return base;
            }
        } else {
            contig = 0;
        }
    }
    return NULL;
}

void free_pages(uint32_t frame, uint32_t count) {
    uint32_t paddr = frame * PAGE_SIZE;
    for(uint32_t i = 0; i < count; i++) {
        clear_frame(paddr + i * PAGE_SIZE);
    }
}

void swap_dir(page_directory_t* dir) {
    // Move the page directory address into the cr3 register
    asm volatile("mov %0, %%cr3":: "r"(KV2P(dir)));
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

void copy_page_table_entries(page_table_t* src, page_table_t* dest) {
    for(int i = 0; i < 1024; i++) {
        dest->pages[i] = src->pages[i];
    }
}

void clone_page_dir(page_directory_t* src, page_directory_t* dest) {
    for(int i = 0; i < 1024; i++) {
        // copy entries for kernel pages, should refer to same physical addresses
        if(i >= KERN_START_TBL) {
            dest->page_dir_entries[i] = src->page_dir_entries[i];
            dest->tables[i] = src->tables[i];
        } else if(src->page_dir_entries[i].frame != 0) {
            // copy the content of all mapped pages into new pages in the new directory
            // TODO: implement as a copy-on-write in conjunction w/ page fault handler
            // allocate a new page table
            page_table_t* dest_page_table = KP2V(alloc_pages(PMM_FLAGS_DEFAULT, 1)); 
            dest->tables[i] = dest_page_table;
            // copy all data except page frame & metadata
            dest->page_dir_entries[i] = src->page_dir_entries[i];
            dest->page_dir_entries[i].frame = PAGE_FRAME(KV2P(dest_page_table));
            dest->page_dir_entries[i].accessed = 0;
            // loop thru pages
            for(int j = 0; j < 1024; j++) {
                // if page mapped
                if(src->tables[i]->pages[j].frame != 0) {
                    // then get the sources virtual address, allocate a new HIGHMEM page and 
                    // temporarily map to lowmem for access
                    void* src_page_vaddr = PAGE_IDX_VADDR(i, j, 0);
                    uint32_t paddr = alloc_pages(PMM_FLAGS_HIGHMEM, 1);
                    void* tmp_dest_page_vaddr = kmap(paddr);
                    memcpy(tmp_dest_page_vaddr, src_page_vaddr, PAGE_SIZE);
                    kunmap(tmp_dest_page_vaddr);
                    // copy all data except meta data & set frame
                    dest->tables[i]->pages[j] = src->tables[i]->pages[j];
                    dest->tables[i]->pages[j].accessed = 0;
                    dest->tables[i]->pages[j].dirty = 0;
                    dest->tables[i]->pages[j].frame = paddr / PAGE_SIZE;
                }
            }
        }
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

void setup_kernel_directory() {
    page_table_t* cur_table;
    for(uint32_t i = KERN_START_TBL; i < 1024; i++) {
        cur_table = &kernel_page_tables[i - KERN_START_TBL];
        kernel_directory->tables[i] = cur_table;
        uint32_t cur_table_paddr = (uint32_t)(cur_table) - 0xC0000000;
        kernel_directory->page_dir_entries[i].frame = (uint32_t)(cur_table_paddr) / PAGE_SIZE;
        kernel_directory->page_dir_entries[i].present = 1;
        kernel_directory->page_dir_entries[i].rw = 1;
        for(int j = 0; j < 1024; j++) {
            // only set frame if should be id mapped (between 0MiB & 896MiB)
            // highmem reserved for phys mapping, large virtually contig buffers, etc...
            if(i < KERN_HIGHMEM_START_TBL) {
                cur_table->pages[j].frame = (i - KERN_START_TBL) * 1024 + j;
                cur_table->pages[j].present = 1;
                cur_table->pages[j].rw = 1;
            }
        }
    }
}

void set_page(page_t* page, uint32_t frame, bool present, bool rw, bool user) {
    page->frame = frame;
    page->present = present;
    page->rw = rw;
    page->user = user;
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
    swap_dir(kernel_directory);
    terminal_initialize();
    reserve_mem_map(mbd);
    reserve(0, PAGE_TABLE_SIZE);
}
