#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <core/syscall.h>
#include <core/isr.h>
#include <proc/proc.h>
#include <mm/paging.h>

syscall_handler_t syscall_table[SYSCALL_MAX];

bool user_addr_accessible(const proc_t* proc, uint32_t addr) {
    if(proc == NULL || proc->page_directory == NULL) {
        return false;
    }
    if(addr >= (uint32_t)PAGE_IDX_VADDR((uint32_t)KERN_START_TBL, 0, 0)) {
        return false;
    }

    page_directory_t* dir = proc->page_directory;
    uint32_t pd_idx = PAGE_DIR_IDX(addr);
    uint32_t pt_idx = PAGE_TBL_IDX(addr);

    page_dir_entry_t entry = dir->page_dir_entries[pd_idx];
    if(!entry.present || !entry.user) {
        return false;
    }

    page_table_t* table = dir->tables[pd_idx];
    if(table == NULL) {
        return false;
    }

    page_t page = table->pages[pt_idx];
    if(!page.present || !page.user) {
        return false;
    }

    return true;
}

static bool copy_user_string(proc_t* proc, char* dest, size_t dest_len, const char* src, uint32_t user_len) {
    if(dest == NULL || dest_len == 0 || src == NULL || proc == NULL) {
        return false;
    }

    size_t limit = user_len;
    if(limit >= dest_len) {
        limit = dest_len - 1;
    }
    if(limit == 0) {
        dest[0] = '\0';
        return true;
    }

    size_t copied = 0;
    while(copied < limit) {
        uint32_t addr = (uint32_t)src + (uint32_t)copied;
        if(!user_addr_accessible(proc, addr)) {
            return false;
        }

        char c = *((const volatile char*)addr);
        dest[copied++] = c;
        if(c == '\0') {
            return true;
        }
    }

    dest[copied] = '\0';
    return true;
}

int syscall_register(uint32_t num, syscall_handler_t handler) {
    if(num >= SYSCALL_MAX || handler == NULL) {
        return -1;
    }
    if(syscall_table[num] != NULL) {
        return -2;
    }

    syscall_table[num] = handler;
    return 0;
}

void syscall_init(void) {
    memset(syscall_table, 0, sizeof(syscall_table));
    isr_set_handler(SYSCALL_VECTOR, syscall_dispatch);
    int rc = syscall_register(SYS_PRINT_STRING, sys_print_string);
    if(rc != 0) {
        printf("syscall_init: failed to register SYS_PRINT_STRING (%d)\n", rc);
    }
}

void syscall_dispatch(int_regs_t* regs) {
    if(regs == NULL) {
        return;
    }

    uint32_t num = regs->eax;
    if(num < SYSCALL_MAX) {
        syscall_handler_t handler = syscall_table[num];
        if(handler != NULL) {
            handler(regs);
            return;
        }
    }

    regs->eax = (uint32_t)-1;
    printf("syscall: unhandled number %u\n", num);
}

void sys_print_string(int_regs_t* regs) {
    printf("printf syscall called from process %u\n", current_proc ? current_proc->pid : 0);
    if(regs == NULL) {
        return;
    }
    if(current_proc == NULL) {
        regs->eax = (uint32_t)SYSCALL_EINVAL;
        return;
    }

    const char* user_buf = (const char*)regs->ebx;
    uint32_t user_len = regs->ecx;

    if(user_buf == NULL || user_len == 0) {
        regs->eax = (uint32_t)SYSCALL_EINVAL;
        return;
    }

    char buffer[SYS_PRINT_STRING_MAX_LEN];
    if(!copy_user_string(current_proc, buffer, sizeof(buffer), user_buf, user_len)) {
        regs->eax = (uint32_t)SYSCALL_EFAULT;
        return;
    }

    //printf("[%u] %s\n", current_proc->pid, buffer);
    printf("%s\n", buffer);
    regs->eax = (uint32_t)SYSCALL_SUCCESS;
}
