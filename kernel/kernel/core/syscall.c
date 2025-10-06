#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <core/syscall.h>
#include <core/isr.h>
#include <drivers/tty.h>
#include <mm/paging.h>
#include <mm/vmm.h>
#include <proc/proc.h>

static int sys_write(uint32_t fd, const char* buf, size_t len) {
    if (fd != 1 || buf == NULL) return -1;
    // For initial bring-up: trust user pointer in current address space.
    // Clamp to a reasonable amount to avoid runaway prints.
    if (len > 1 << 20) len = 1 << 20; // 1 MiB cap
    terminal_write(buf, len);
    return (int)len;
}

static uintptr_t page_align_down(uintptr_t v) { return v & ~(PAGE_SIZE - 1); }
static uintptr_t page_align_up(uintptr_t v) { return (v + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); }

static int map_user_page(page_directory_t* dir, uintptr_t vaddr, uint32_t paddr) {
    uint32_t pd_idx = PAGE_DIR_IDX(vaddr);
    uint32_t pt_idx = PAGE_TBL_IDX(vaddr);

    if (!dir->page_dir_entries[pd_idx].present) {
        page_table_t* new_table = (page_table_t*)KP2V(alloc_pages(PMM_FLAGS_DEFAULT, 1));
        if (!new_table) return -1;
        memset(new_table, 0, sizeof(page_table_t));
        dir->tables[pd_idx] = new_table;
        dir->page_dir_entries[pd_idx].present = 1;
        dir->page_dir_entries[pd_idx].rw = 1;
        dir->page_dir_entries[pd_idx].user = 1;
        dir->page_dir_entries[pd_idx].frame = PAGE_FRAME(KV2P((uint32_t)new_table));
    }

    set_page(&dir->tables[pd_idx]->pages[pt_idx], PAGE_FRAME(paddr), 1, 1, 1);
    return 0;
}

static void unmap_user_page(page_directory_t* dir, uintptr_t vaddr) {
    uint32_t pd_idx = PAGE_DIR_IDX(vaddr);
    uint32_t pt_idx = PAGE_TBL_IDX(vaddr);
    if (!dir->page_dir_entries[pd_idx].present) return;
    page_t* p = &dir->tables[pd_idx]->pages[pt_idx];
    // Free backing frame if present
    if (p->frame) {
        free_pages(p->frame, 1);
    }
    // Clear entry
    *((uint32_t*)p) = 0;
}

static void* sys_brk(void* new_end) {
    proc_t* p = current_proc;
    if (!p) return (void*)-1;
    if (new_end == 0) return p->brk; // query current brk

    uintptr_t old = (uintptr_t)p->brk;
    uintptr_t start = (uintptr_t)p->heap_start;
    uintptr_t target = (uintptr_t)new_end;
    if (target < start) target = start; // never contract below heap start

    page_directory_t* dir = p->page_directory;
    uintptr_t old_al = page_align_up(old);
    uintptr_t new_al = page_align_up(target);

    if (new_al > old_al) {
        // Grow: map new pages
        for (uintptr_t v = old_al; v < new_al; v += PAGE_SIZE) {
            uint32_t phys = alloc_pages(PMM_FLAGS_HIGHMEM, 1);
            if (!phys) return (void*)-1;
            if (map_user_page(dir, v, phys) != 0) return (void*)-1;
            // Zero page via user address (current CR3 maps it now)
            memset((void*)v, 0, PAGE_SIZE);
        }
    } else if (new_al < old_al) {
        // Shrink: unmap pages strictly beyond new_al
        for (uintptr_t v = new_al; v < old_al; v += PAGE_SIZE) {
            unmap_user_page(dir, v);
        }
    }

    p->brk = (void*)target;
    return p->brk;
}

static void* sys_sbrk(intptr_t incr) {
    proc_t* p = current_proc;
    if (!p) return (void*)-1;
    uintptr_t old = (uintptr_t)p->brk;
    void* res = sys_brk((void*)(old + incr));
    if (res == (void*)-1) return res;
    return (void*)old;
}

static int sys_getpid(void) {
    return current_proc ? (int)current_proc->pid : -1;
}

static int sys_exit(int code) {
    (void)code;
    printf("Process %d exited. Halting.\n", current_proc ? current_proc->pid : -1);
    // Naive: halt CPU
    for(;;) { asm volatile("hlt"); }
}

static void syscall_handler(int_regs_t* r) {
    uint32_t nr = r->eax;
    switch (nr) {
        case SYS_write:
            r->eax = (uint32_t)sys_write(r->ebx, (const char*)r->ecx, r->edx);
            break;
        case SYS_brk:
            r->eax = (uint32_t)sys_brk((void*)r->ebx);
            break;
        case SYS_sbrk:
            r->eax = (uint32_t)sys_sbrk((intptr_t)r->ebx);
            break;
        case SYS_getpid:
            r->eax = (uint32_t)sys_getpid();
            break;
        case SYS_exit:
            r->eax = (uint32_t)sys_exit(r->ebx);
            break;
        default:
            r->eax = (uint32_t)-1;
            break;
    }
}

void syscall_init(void) {
    isr_set_handler(128, &syscall_handler);
}

