#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <mm/paging.h>
#include <mm/vmm.h>
#include <core/common.h>

uint32_t pid_inc() {
    spinlock_acquire(&pid_lock);
    uint32_t ret = pid_counter++;
    spinlock_release(&pid_lock);
    return ret;
}

void copy_page_dir_cow(page_directory_t* src, page_directory_t* dest) {
    for(int i = 0; i < 1024, i++) {
        src->page_dir_entries[i].rw = 0;
        dest->page_dir_entries[i] = src->page_dir_entries[i];
    }
    for(int i = 0; i < 1024; i++) {
        dest->tables[i] = src->tables[i];
    }
}