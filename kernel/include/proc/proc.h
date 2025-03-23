#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <mm/paging.h>
#include <core/common.h>

#define MAXPROC 64

struct context {
    uint32_t esp;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t esi;
    uint32_t edi;
    uint32_t eip;
} __attribute__((packed));

typedef struct context proc_context_t;

enum procstate {
    PROC_UNUSED,
    PROC_SETUP,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_DESTROY
}

typedef enum procstate procstate_t;

struct proc {
    uint32_t pid;
    page_directory_t* page_directory;
    proc_context_t* context;
    procstate_t procstate;
}

void copy_page_dir_cow(page_directory_t* src, page_directory_t* dest);
