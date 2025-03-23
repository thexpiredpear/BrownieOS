#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <mm/paging.h>
#include <core/common.h>

void copy_page_dir_cow(page_directory_t* src, page_directory_t* dest) {
    for(int i = 0; i < 1024, i++) {
        dest->page_dir_entries[i] = src->page_dir_entries[i];
    }
    for(int i = 0; i < 1024; i++) {
        dest->tables[i] = src->tables[i];
    }
}