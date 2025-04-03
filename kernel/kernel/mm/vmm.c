#include <stddef.h>
#include <stdint.h>
#include <mm/vmm.h>
#include <mm/paging.h>

void* kmap(uint32_t paddr) {
    if(paddr == 0) {
        return NULL;
    } else if(paddr < 0x40000000) {
        return (void*)KP2V(paddr);
    } else {
        page_directory_t* dir = get_current_directory();
        for(uint32_t i = KERN_HIGHMEM_START_TBL; i < 1024; i++) {
            page_table_t* table = dir->tables[i];
            for(uint32_t j = 0; j < 1024; j++) {
                // find first free page in highmem
                if(*((uint32_t*)(&table->pages[j])) == 0) {
                    set_page(&table->pages[j], PAGE_FRAME(paddr), 1, 1, 0);
                    return (void*)PAGE_IDX_VADDR(i, j, paddr % PAGE_SIZE);
                }
            }
        }
    }
}

void kunmap(void* vaddr) {
    if(vaddr == 0) {
        return;
    } else if((uint32_t)vaddr < 0xC0000000) {
        return;
    } else {
        page_directory_t* dir = get_current_directory();
        uint32_t table = PAGE_DIR_IDX((uint32_t)vaddr);
        uint32_t page = PAGE_TBL_IDX((uint32_t)vaddr);
        *(uint32_t*)(&(dir->tables[table]->pages[page])) = 0;
    }
}