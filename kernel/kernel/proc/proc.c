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
proc_t* proc_list[MAXPROC];
extern page_directory_t* kernel_directory;

void proc_init(void) {
    for (int i = 0; i < MAXPROC; i++) {
        proc_list[i] = NULL;
    }
    current_proc = NULL;
}

void kernel_proc_init(void) {
    proc_t* kernel_proc = (proc_t*)kmalloc(sizeof(proc_t));
    kernel_proc->pid = pid_ctr++; // PID 0
    kernel_proc->procstate = PROC_RUNNING;
    kernel_proc->priority = PROC_PRIORITY_HIGH;
    kernel_proc->page_directory = kernel_directory;
    // Kernel stack and heap are managed differently, so these might be NULL or set to specific kernel values.
    // For now, let's leave them as is or set to 0/NULL.
    kernel_proc->brk = NULL;
    kernel_proc->heap_start = NULL;
    kernel_proc->stack_top = NULL; 
    kernel_proc->stack_size = 0; 
    // Context for the kernel process is not switched to/from in the same way.
    // It's effectively always "running" on CPU 0 initially.
    // memset(&kernel_proc->context, 0, sizeof(proc_context_t)); 

    proc_list[kernel_proc->pid] = kernel_proc;
    current_proc = kernel_proc;
    printf("Kernel process initialized with PID %d\n", kernel_proc->pid);
}

proc_t* create_proc(void* entry, uint32_t exec_size, uint32_t stack_size, uint32_t heap_size, procpriority_t priority) {
    proc_t* proc = NULL;
    int proc_idx = -1;

    // Find an empty slot in the process list
    // Start from 1 because PID 0 is for the kernel
    for (int i = 1; i < MAXPROC; i++) {
        if (proc_list[i] == NULL || proc_list[i]->procstate == PROC_UNUSED) {
            if (proc_list[i] != NULL && proc_list[i]->procstate == PROC_UNUSED) {
                // If a slot was previously used and is now UNUSED, free its old resources if any
                // For now, we assume kmalloc'd proc_t is the main resource to free if re-using.
                // More complex cleanup (page tables etc.) would go here if re-using PROC_UNUSED slots
                // that previously held a terminated process.
                kfree(proc_list[i]); 
            }
            proc = (proc_t*)kmalloc(sizeof(proc_t));
            if (!proc) {
                printf("create_proc: kmalloc failed for proc_t\n");
                return NULL; // kmalloc failed
            }
            proc_idx = i;
            break;
        }
    }

    if (proc_idx == -1) {
        printf("create_proc: Max processes reached\n");
        return NULL; // No free slot
    }

    proc->pid = pid_ctr++; // Assign next available PID
    proc->procstate = PROC_SETUP;
    proc->priority = priority;

    proc->page_directory = (page_directory_t*)KP2V(alloc_pages(PMM_FLAGS_DEFAULT, 2));
    if (!proc->page_directory) {
        printf("create_proc: alloc_pages failed for page directory\n");
        kfree(proc);
        return NULL;
    }
    memset(proc->page_directory, 0, PAGE_SIZE * 2); // Clear the allocated pages

    clone_page_dir(kernel_directory, proc->page_directory);

    uint32_t stack_top = PROC_STACK_TOP;
    uint32_t stack_bottom = stack_top - stack_size;

    uint32_t stack_pages = ((stack_top + 0xFFF) & ~0xFFF) - (stack_bottom & ~0xFFF) / PAGE_SIZE;
    uint32_t stack_phys = alloc_pages(PMM_FLAGS_HIGHMEM, stack_pages);

    proc->stack_top = (void*)stack_top;
    proc->stack_size = stack_size;

    for(uint32_t i = 0;  i < stack_pages; i++) {
        uint32_t phys = stack_phys + i * PAGE_SIZE;
        uint32_t virt = stack_bottom + i * PAGE_SIZE; // align to page size

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
    proc->context.esp = stack_top - 16; // set stack pointer to top of stack w/ some paddding
    proc->context.ebp = proc->context.esp; // set base pointer to stack pointer

    proc_list[proc_idx] = proc; // Add to process list
    proc->procstate = PROC_RUNNING; // Or PROC_READY if we had a scheduler
    printf("Created process with PID %d, slot %d\n", proc->pid, proc_idx);
    return proc;
}