#ifndef _KERNEL_IOAPIC_H
#define _KERNEL_IOAPIC_H 1

#include <stdint.h>
#include <stdbool.h>

#define IOAPIC_ID_REG 0x00
#define IOAPIC_VER_REG 0x01
#define IOAPIC_ARB_REG 0x02
#define IOAPIC_RED_TBL_REG(n) (0x10 + (2 * n))

struct redir_entry_lo {
    uint32_t vector : 8;
    uint32_t deliv_mode : 3;
    uint32_t dest_mode : 1;
    uint32_t deliv_status : 1;
    uint32_t pin_polarity : 1;
    uint32_t remote_irr : 1;
    uint32_t trigger_mode : 1;
    uint32_t masked : 1;
    uint32_t reserved : 15;
} __attribute__((packed));

typedef struct redir_entry_lo redir_entry_lo_t;

struct redir_entry_hi {
    uint32_t reserved : 24;
    uint32_t dest : 8;
} __attribute__((packed));

typedef struct redir_entry_hi redir_entry_hi_t;

void init_ioapic();

#endif
