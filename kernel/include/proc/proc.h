#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <mm/paging.h>
#include <core/common.h>

#define MAXPROC 64
#define PROC_STACK_TOP 0xBFFFFFF0

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
};

typedef enum procstate procstate_t;

enum procpriority {
    PROC_PRIORITY_LOW,
    PROC_PRIORITY_NORMAL,
    PROC_PRIORITY_HIGH
};

typedef enum procpriority procpriority_t;

typedef uint32_t pid_t;

struct proc {
    pid_t pid;
    page_directory_t* page_directory;
    proc_context_t context;
    procstate_t procstate;
    procpriority_t priority;
    void* brk;
    void* heap_start;
    void* stack_top;
    uint32_t stack_size;
};

typedef struct proc proc_t;

extern proc_t* proc_list[MAXPROC];
extern proc_t* current_proc;

void proc_init(void);
void kernel_proc_init(void);
proc_t* create_proc(void* entry, uint32_t exec_size, uint32_t stack_size, uint32_t heap_size, procpriority_t priority);
