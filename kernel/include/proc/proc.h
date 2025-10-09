#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <mm/paging.h>
#include <core/common.h>
#include <core/isr.h>

#define MAXPROC 64
#define PROC_STACK_TOP 0xBFFFFFF0

// Saved CPU context/trap frame used for process resumes and initial user entry.
// Matches the ordering established by `interrupt.S` for PUSHAL + ISR pushes
// and the CPU-pushed iret frame. Note: the `esp` within the PUSHAD block is
// the KERNEL esp at interrupt time; `useresp`/`ss` are only present for CPL3
// -> CPL0 transitions (user->kernel interrupts) and for user-mode iret frames.
struct context {
    // PUSHAD-saved general purpose registers (top of frame first)
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    // CPU iret frame
    uint32_t eip, cs, eflags;
    uint32_t useresp; // only valid when transitioning to/from ring 3
    uint32_t ss;      // only valid when transitioning to/from ring 3
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
    // Per-process kernel stack used on privilege elevation (TSS.ESP0)
    void* kstack_top;
    uint32_t kstack_size;
    void* kstack_base;
    // Cached CR3 value (physical address of page directory) for fast context switches
    uint32_t cr3;
    // Simple forward link for run queues (placeholder for future scheduler)
    struct proc* run_next;
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
// Placeholder scheduler bootstrap (to be implemented); prepares run queue state.
void scheduler_init(void);
void scheduler_save_current_context(const int_regs_t* regs);
void scheduler_prepare_switch(proc_t* next_proc, int_regs_t* regs);
// Creates a new user process with a private page directory cloned from the
// kernel directory: allocates user stack pages (physical HIGHMEM) and maps them
// into the process address space, sets initial context with `entry` (virtual).
// Returns a kernel virtual pointer to the new `proc_t`, or NULL on failure.
proc_t* create_proc(void* entry, uint32_t exec_size, uint32_t stack_size, uint32_t heap_size, procpriority_t priority);

// Transfers control to user mode for process `p` by switching to its address
// space, updating TSS.ESP0 to its kernel stack, and executing an iret path.
// Requires that `p->context` is initialized appropriately.
void proc_enter(proc_t* p);

// Low-level iret jump to user mode using the provided saved context.
// Does not return. Implemented in assembly.
__attribute__((noreturn)) void iret_jump_user(proc_context_t* ctx);

// Helpers for converting between interrupt register frames and stored contexts.
void proc_context_from_regs(proc_context_t* dest, const int_regs_t* src);
void proc_context_to_regs(int_regs_t* dest, const proc_context_t* src);
