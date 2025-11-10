#ifdef _KERNEL_SCHEDULER_H
#define _KERNEL_SCHEDULER_H 1

#include <stdint.h>
#include <proc/proc.h>

void scheduler_init(void);
void scheduler_prepare_switch(proc_t* next_proc, int_regs_t* regs);
void scheduler_save_current_context(const int_regs_t* regs);

#endif