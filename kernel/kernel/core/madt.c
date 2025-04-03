#include <cpuid.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <core/madt.h>
#include <core/apic.h>
#include <core/acpi.h>
#include <core/multiboot.h>
#include <core/common.h>
#include <mm/kmm.h>

uint8_t num_procs = 0;
uint8_t lapic_ids[256];
uint8_t num_iso_entries = 0;
ioapic_iso_t iso_entries[256];
uint32_t lapic_addr = 0;
uint32_t ioapic_addr = 0;

void parse_madt() {
    madt_t* madt = get_madt();
    uint8_t* ptr = madt->records;
    printf("LAPIC ADDRESS: %x\n", madt->lapic_addr);
    lapic_addr = madt->lapic_addr;
    printf("--------- MADT ENTRIES ---------\n");
    while((uint32_t)ptr < (uint32_t)madt + madt->header.length) {
        uint32_t type = *ptr;
        switch(type) {
            case LAPIC:
                char* enabled;
                char* online;
                lapic_t* lapic = (lapic_t*)ptr;
                if(lapic->flags & 0b1) {
                    enabled = "enabled";
                } else {
                    enabled = "disabled";
                }
                if(lapic->flags & 0b10) {
                    online = "online";
                } else {
                    online = "offline";
                }
                char str[33];
                utoa(lapic->flags, str, 2);
                printf("LAPIC: acpi id %d, apic id %d, flags %s, %s, %s\n", lapic->acpi_id, lapic->apic_id, str, enabled, online);
                lapic_ids[num_procs++] = lapic->apic_id;
                ptr += lapic->header.length;
                break;
            case IOAPIC:
                ioapic_t* ioapic = (ioapic_t*)ptr;
                printf("IOAPIC: id %d, addr 0x%x, gsi base %d\n", ioapic->id, ioapic->addr, ioapic->gsi_base);
                if(ioapic_addr == 0) {
                    ioapic_addr = ioapic->addr;
                }
                ptr += ioapic->header.length;
                break;
            case IOAPIC_ISO:
                ioapic_iso_t* iso = (ioapic_iso_t*)ptr;
                printf("IOAPIC ISO: irq %d, gsi %d, flags %x\n", iso->irq_src, iso->gsi, iso->flags);
                iso_entries[num_iso_entries++] = *iso;
                ptr += iso->header.length;
                break;
            case NMI_SRC:
                nmi_src_t* nmi = (nmi_src_t*)ptr;
                printf("NMI SRC: irq %d, gsi %d, flags %x\n", nmi->nmi_src, nmi->gsi, nmi->flags);
                ptr += nmi->header.length;
                break;
            case LAPIC_NMI:
                lapic_nmi_t* lapic_nmi = (lapic_nmi_t*)ptr;
                printf("LAPIC NMI: acpi id %d, flags %x, lint %d\n", lapic_nmi->acpi_id, lapic_nmi->flags, lapic_nmi->lint);
                ptr += lapic_nmi->header.length;
                break;
            case LAPIC_ADDR_OVR:
                lapic_addr_ovr_t* lapic_addr_ovr = (lapic_addr_ovr_t*)ptr;
                printf("LAPIC ADDR OVR: addr 0x%llx\n", lapic_addr_ovr->addr);
                ptr += lapic_addr_ovr->header.length;
                break;
        }
    }
    printf("--------- END MADT ENTRIES ---------\n");
    return;
}

uint8_t get_num_procs() {
    return num_procs;
}

uint8_t* get_lapic_ids() {
    return lapic_ids;
}

uint32_t get_lapic_addr() {
    return lapic_addr;
}

uint32_t get_ioapic_addr() {
    return ioapic_addr;
}

ioapic_iso_t* get_iso_entries() {
    return iso_entries;
}