#include <cpuid.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <core/madt.h>
#include <core/ioapic.h>
#include <core/acpi.h>
#include <core/multiboot.h>
#include <core/common.h>
#include <mm/kmm.h>

uint32_t ioapic = 0;
uint8_t ioapic_ver = 0;
uint8_t max_redir_entries = 0;
uint8_t gsi_count = 0;
ioapic_iso_t* gsi_entries = NULL;

void write_ioapic(uint32_t reg, uint32_t val) {
    // set IOREGSEL to reg
    *(volatile uint32_t*)(ioapic) = reg;
    // write val to IOREGWIN
    *(volatile uint32_t*)(ioapic + 0x10) = val;
}

uint32_t read_ioapic(uint32_t reg) {
    // set IOREGSEL to reg
    *(volatile uint32_t*)(ioapic) = reg;
    // read from IOREGWIN
    return *(volatile uint32_t*)(ioapic + 0x10);
}

void init_ioapic() {
    ioapic = (uint32_t)KP2V((uint32_t)get_ioapic_addr());
    gsi_entries = get_iso_entries();
    printf("phys addr: %x\n", get_ioapic_addr());
    ioapic_ver = read_ioapic(IOAPIC_VER_REG) & 0xFF;
    max_redir_entries = ((read_ioapic(IOAPIC_VER_REG) >> 16) & 0xFF) + 1;
    for(uint8_t i = 0; i < max_redir_entries; i++) {
        uint8_t lo_idx = IOAPIC_RED_TBL_REG(i);
        uint8_t hi_idx = IOAPIC_RED_TBL_REG(i) + 1;
        redir_entry_lo_t entry_lo;
        redir_entry_hi_t entry_hi;
        uint32_t lo = read_ioapic(lo_idx);
        uint32_t hi = read_ioapic(hi_idx);
        if(i == gsi_entries[gsi_count].gsi) {
            entry_lo.vector = 32 + gsi_entries[gsi_count].irq_src;
            gsi_count++;
        } else {
            entry_lo.vector = 32 + i;
        }
        entry_lo.deliv_mode = 0;
        entry_lo.dest_mode = 0;
        entry_lo.deliv_status = 0;
        entry_lo.pin_polarity = 0;
        entry_lo.trigger_mode = 0;
        entry_lo.masked = 0;
        entry_lo.reserved = ((redir_entry_lo_t*)&lo)->reserved;
        entry_hi.reserved = ((redir_entry_hi_t*)&hi)->reserved;
        lo = *(uint32_t*)&entry_lo;
        hi = *(uint32_t*)&entry_hi;
        write_ioapic(lo_idx, lo);
        write_ioapic(hi_idx, hi);
        lo = read_ioapic(lo_idx);
        hi = read_ioapic(hi_idx);
        entry_lo = *(redir_entry_lo_t*)&lo;
        entry_hi = *(redir_entry_hi_t*)&hi;
        int j = i;
    }
    printf("IOAPIC: addr 0x%x, ioapic ver %d, max redir entries %d\n", ioapic, ioapic_ver, max_redir_entries);
}