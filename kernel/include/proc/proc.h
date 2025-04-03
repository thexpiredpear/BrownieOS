#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <mm/paging.h>
#include <core/common.h>

#define MAXPROC 64

struct context {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
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

enum procpriority {
    PROC_PRIORITY_LOW,
    PROC_PRIORITY_NORMAL,
    PROC_PRIORITY_HIGH
}

typedef enum procpriority procpriority_t;

typedef uint32_t pid_t;

struct proc {
    pit_t pid;
    page_directory_t* page_directory;
    proc_context_t* context;
    procstate_t procstate;
}

void (page_directory_t* src, page_directory_t* dest);
