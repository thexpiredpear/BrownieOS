#ifndef _KERNEL_SCHEDULER_H
#define _KERNEL_SCHEDULER_H 1

#include <stdint.h>
#include <proc/proc.h>
#include <core/isr.h>

// Default RR timeslice in seconds (at PIT frequency). Always enabled.
#ifndef SCHED_SLICE_SECONDS
#define SCHED_SLICE_SECONDS 2u
#endif

void scheduler_init(void);
// Immediate yield from an interrupted state: saves current and switches to the
// next runnable user process, if any.
void scheduler_switch_next(int_regs_t* regs);
void scheduler_switch_process(proc_t* next, int_regs_t* regs);

proc_t* pick_next_proc(proc_t* cur);

#endif
