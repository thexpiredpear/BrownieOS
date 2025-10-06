#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <mm/paging.h>
#include <mm/vmm.h>
#include <proc/proc.h>
#include <core/syscall.h>

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

int exec_load_demo(proc_t* p) {
    const uintptr_t base = 0x01000000; // 16 MiB
    // Tiny user program:
    //   write(1, msg, len);
    //   exit(0);
    const char* msg = "Hello from user!\n";
    const uint32_t msg_len = (uint32_t)strlen(msg);

    uint8_t code[64];
    uint32_t pos = 0;
    #define EMIT8(b)   do { code[pos++] = (uint8_t)(b); } while(0)
    #define EMIT32(w)  do { uint32_t __w=(w); memcpy(&code[pos], &__w, 4); pos+=4; } while(0)

    // mov eax, SYS_write
    EMIT8(0xB8); EMIT32(SYS_write);
    // mov ebx, 1 (fd)
    EMIT8(0xBB); EMIT32(1);
    // mov ecx, msg_addr (patched later)
    EMIT8(0xB9); uint32_t msg_ptr_pos = pos; EMIT32(0);
    // mov edx, len
    EMIT8(0xBA); EMIT32(msg_len);
    // int 0x80
    EMIT8(0xCD); EMIT8(0x80);
    // exit(0)
    EMIT8(0xB8); EMIT32(SYS_exit);
    EMIT8(0xBB); EMIT32(0);
    EMIT8(0xCD); EMIT8(0x80);

    // Patch message pointer
    uint32_t msg_addr = (uint32_t)(base + pos);
    memcpy(&code[msg_ptr_pos], &msg_addr, 4);

    uint32_t total = pos + msg_len;
    uint32_t pages = (page_align_up(base + total) - base) / PAGE_SIZE;
    if (pages == 0) pages = 1;

    // Allocate and map pages
    for (uint32_t i = 0; i < pages; i++) {
        uint32_t phys = alloc_pages(PMM_FLAGS_HIGHMEM, 1);
        if (!phys) return -1;
        if (map_user_page(p->page_directory, base + i * PAGE_SIZE, phys) != 0) return -1;
        // Zero via temporary kernel mapping then unmap
        void* kv = kmap(phys);
        if (!kv) return -1;
        memset(kv, 0, PAGE_SIZE);
        kunmap(kv);
    }

    // Copy code and message into user memory (same CR3 when we enter; for now
    // we can use kmap on frames again for simplicity): copy page by page.
    // For simplicity, memcpy directly to user VA; this will work when the
    // process is active. Here we rely on pages being present in p->dir but not
    // yet active; so map-and-copy via kmap.
    // We'll map and write per page using kmap again.
    // First page: write code and possibly part of msg
    uint32_t remaining = total;
    uint32_t copied = 0;
    uint32_t off = 0;
    while (remaining) {
        uint32_t v = base + copied;
        uint32_t pd_idx = PAGE_DIR_IDX(v);
        uint32_t pt_idx = PAGE_TBL_IDX(v);
        page_t* pt = &p->page_directory->tables[pd_idx]->pages[pt_idx];
        uint32_t phys = PAGE_PADDR(pt->frame);
        void* kv = kmap(phys);
        if (!kv) return -1;
        uint32_t page_off = v & (PAGE_SIZE - 1);
        uint32_t chunk = PAGE_SIZE - page_off;
        if (chunk > remaining) chunk = remaining;
        // Source pointer inside combined buffer
        if (copied < pos) {
            uint32_t cpy = chunk;
            if (copied + cpy > pos) cpy = pos - copied;
            memcpy((uint8_t*)kv + page_off, code + copied, cpy);
            if (chunk > cpy) {
                // tail from message
                memcpy((uint8_t*)kv + page_off + cpy, msg, chunk - cpy);
            }
        } else {
            uint32_t msg_off = copied - pos;
            memcpy((uint8_t*)kv + page_off, msg + msg_off, chunk);
        }
        kunmap(kv);
        copied += chunk;
        remaining -= chunk;
        (void)off;
    }

    // Set entry and initial heap
    p->context.eip = base;
    p->heap_start = (void*)page_align_up(base + total);
    p->brk = p->heap_start;
    return 0;
}

