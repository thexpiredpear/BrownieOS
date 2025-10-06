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
    void* kstack_top;   // per-process ring 0 stack top (for syscalls/IRQs)
    uint32_t kstack_size;
};

typedef struct proc proc_t;

// Global process table: array of pointers to process control blocks (PCBs).
// Each `proc_t*` is a kernel virtual pointer; slots may be NULL or point to a
// `proc_t` whose `procstate` indicates its lifecycle. Indexed by PID for now.
extern proc_t* proc_list[MAXPROC];
// Pointer to the currently running process’s PCB (kernel virtual pointer). May
// be NULL during early boot or before the scheduler is initialized.
extern proc_t* current_proc;

// Initializes the process subsystem’s globals (`proc_list`, `current_proc`).
// Does not allocate or create any processes; meant to be called at boot.
void proc_init(void);
// Creates and registers PID 0 as the kernel process. Sets `current_proc` and
// points its `page_directory` at the global kernel directory (shared address space).
void kernel_proc_init(void);
// Creates a new user process with a private page directory cloned from the
// kernel directory: allocates user stack pages (physical HIGHMEM) and maps them
// into the process address space, sets initial context with `entry` (virtual).
// Returns a kernel virtual pointer to the new `proc_t`, or NULL on failure.
proc_t* create_proc(void* entry, uint32_t exec_size, uint32_t stack_size, uint32_t heap_size, procpriority_t priority);

// Enters user mode for process `p`: switches to its address space, updates TSS
// ESP0 to the process kernel stack, and iret's to user eip/esp.
void proc_enter(proc_t* p);

// Loads a minimal demo program into process `p` at 0x01000000 that calls
// write and exit via int 0x80. Sets initial heap (brk) to end of image.
int exec_load_demo(proc_t* p);

// Assembly helper to iret into ring 3 with provided EIP/ESP/EFLAGS.
extern void iret_to_user(uint32_t eip, uint32_t esp, uint32_t eflags);
