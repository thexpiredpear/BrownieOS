#include <stdint.h>
#include <stdbool.h>
#include <proc/proc.h>
#include <proc/scheduler.h>
#include <mm/paging.h>
#include <core/tss.h>

// Simple round-robin scheduler driven by the PIT (IRQ0).
//
// Design:
// - Only preempts when the interrupt came from user mode (CPL=3)
// - Saves current user context from `regs` into current_proc->context
// - Picks next runnable user process from proc_list[] (skip NULL, kernel/CPL0,
//   and non-running states)
// - Switches CR3 (swap_dir) and TSS.ESP0 to the next process's kernel stack
// - Overwrites the interrupt frame `regs` with next->context so that IRET
//   returns into the selected process

proc_t* pick_next_proc(proc_t* cur) {
    // Find the current index in the global proc list, then scan forward.
    int cur_idx = -1;
    for (int i = 0; i < MAXPROC; ++i) {
        if (proc_list[i] == cur) { cur_idx = i; break; }
    }

    // Start scanning from the next slot; wrap around once.
    for (int pass = 0; pass < 2; ++pass) {
        int start = (cur_idx >= 0) ? (cur_idx + 1) : 0;
        for (int i = start; i < MAXPROC; ++i) {
            proc_t* p = proc_list[i];
            if (!p) continue;
            if (p == cur) continue;
            if (p->procstate != PROC_RUNNING) continue;
            // Only schedule user mode processes (CPL=3)
            if ((p->context.cs & 0x3) != 0x3) continue;
            return p;
        }
        // wrap
        cur_idx = -1;
    }
    return NULL;
}

void scheduler_init(void) { }

void scheduler_switch_process(proc_t* next, int_regs_t* regs) {
    if (!next || next == current_proc) return;

    proc_context_from_regs(&current_proc->context, regs);

    tss_set_kernel_stack((uint32_t)next->kstack_top);
    swap_dir(next->page_directory);
    current_proc = next;
    printf("Scheduler: switching to process #%u\n", current_proc->pid);

    // Put the next process's context into regs, so on iret we  next.
    proc_context_to_regs(regs, &next->context);
}

// Immediate yield from an ISR context (e.g., explicit call or timeslice expiry).
void scheduler_switch_next(int_regs_t* regs) {
    // Only switch if interrupt came from user mode (CPL=3)
    if ((regs->cs & 0x3) != 0x3) return;

    // Save current user context
    proc_context_from_regs(&current_proc->context, regs);

    // Choose next runnable process
    proc_t* next = pick_next_proc(current_proc);
    scheduler_switch_process(next, regs);
}
