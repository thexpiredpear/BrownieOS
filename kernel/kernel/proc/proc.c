#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <mm/paging.h>
#include <core/common.h>
#include <proc/proc.h>
#include <mm/kmm.h>
#include <string.h>

uint32_t pid_ctr = 0;
proc_t* current_proc;
extern page_directory_t* kernel_directory;

proc_t* proc_init(void* entry, uint32_t stack_size, procpriority_t priority) {
    proc_t* proc = (proc_t*)kmalloc(sizeof(proc_t));

    proc->pid = ++pid_ctr;
    proc->procstate = PROC_SETUP;
    proc->priority = priority;

    proc->page_directory = (page_directory_t*)kmalloc(sizeof(page_directory_t));

    clone_page_dir(kernel_directory, proc->page_directory);

    uint32_t stack_pages = (stack_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t stack_phys = alloc_pages(PMM_FLAGS_HIGHMEM, stack_pages);

    uint32_t stack_base = 0xC0000000 - PAGE_SIZE;
    uint32_t stack_end = stack_base - stack_size; 

    proc->stack_base = (void*)stack_base;
    proc->stack_size = stack_size;

    for(uint32_t i = 0;  i < stack_pages; i++) {
        uint32_t phys = stack_phys + i * PAGE_SIZE;
        uint32_t virt = stack_base + i * PAGE_SIZE;

        uint32_t pd_idx = PAGE_DIR_IDX(virt);
        uint32_t pt_idx = PAGE_TBL_IDX(virt);

        // create page table if it doesn't exist
        if(!proc->page_directory->page_dir_entries[pd_idx].present) {
            page_table_t* new_table = (page_table_t*)KP2V(alloc_pages(PMM_FLAGS_DEFAULT, 1));
            memset(new_table, 0, sizeof(page_table_t));
            proc->page_directory->tables[pd_idx] = new_table;

            proc->page_directory->page_dir_entries[pd_idx].present = 1;
            proc->page_directory->page_dir_entries[pd_idx].rw = 1;
            proc->page_directory->page_dir_entries[pd_idx].frame = PAGE_FRAME(KV2P((uint32_t)new_table));
            proc->page_directory->page_dir_entries[pd_idx].user = 1;
        }

        set_page(&(proc->page_directory->tables[pd_idx]->pages[pt_idx]),
                 PAGE_FRAME(phys), 1, 1, 1);
    }

    proc->context.eip = (uint32_t)entry;
    proc->context.esp = stack_base - 16; // set stack pointer to top of stack w/ some paddding
    proc->context.ebp = proc->context.esp; // set base pointer to stack pointer
}