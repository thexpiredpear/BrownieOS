#include <cpuid.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <core/acpi.h>
#include <core/multiboot.h>
#include <core/common.h>
#include <mm/vmm.h>
#include <mm/kheap.h>

#define RSDP_SIG "RSD PTR "
#define MADT_SIG "APIC"
#define HPET_SIG "HPET"
#define FADT_SIG "FACP"

int acpi_version;

rsdp_t* rsdp_ptr = NULL;
rsdp_t* rsdp = NULL;

rsdt_t* rsdt_ptr = NULL;
rsdt_t* rsdt = NULL;

xsdt_t* xsdt_ptr = NULL;
xsdt_t* xsdt = NULL;

madt_t* madt_ptr = NULL;
madt_t* madt = NULL;

hpet_t* hpet_ptr = NULL;
hpet_t* hpet = NULL;

fadt_t* fadt_ptr = NULL;
fadt_t* fadt = NULL;

void find_rsdp() {
    uint32_t* edba_ptr_ptr = (uint32_t*)(access_paddr_DANGER(0x40E));
    // TODO: actually search the ebda space properly 
    clraccess_paddr_DANGER(edba_ptr_ptr);
    uint64_t* ptr = (uint64_t*)0xC00E0000;
    while((uint32_t)ptr < 0xC00FFFFF) {
        if(strncmp((char*)ptr, RSDP_SIG, 8) == 0) {
            rsdp_ptr = (rsdp_t*)ptr;
            rsdp = kmalloc(sizeof(rsdp_t));
            *rsdp = *rsdp_ptr;
            acpi_version = rsdp->revision;
            if(!rsdp_checksum()) {
                rsdp_ptr = NULL;
                *rsdp = (rsdp_t){0};
                ptr = (uint64_t*)((uint32_t)ptr + 16);
                continue;
            }
            return;
        }
        ptr = (uint64_t*)((uint32_t)ptr + 16);
    }
    return;
}

void find_rsdt_xsdt() {
    if(acpi_version < 2) {
        rsdt_ptr = (rsdt_t*)access_paddr_DANGER(rsdp->rsdt_addr);
        rsdt = kmalloc(rsdt_ptr->header.length);
        memcpy(rsdt, rsdt_ptr, rsdt_ptr->header.length);
        if(!acpi_sdt_checksum((acpi_sdt_header_t*)rsdt_ptr)) {
            panic("rsdt src checksum failed");
        } else if(!acpi_sdt_checksum((acpi_sdt_header_t*)rsdt)) {
            panic("rsdt dst checksum failed");
        }
        clraccess_paddr_DANGER(rsdt_ptr);
    } else {
        xsdt_ptr = (xsdt_t*)access_paddr_DANGER(rsdp->xsdt_addr);
        xsdt = kmalloc(xsdt_ptr->header.length);
        memcpy(xsdt, xsdt_ptr, xsdt_ptr->header.length);
        if(!acpi_sdt_checksum((acpi_sdt_header_t*)xsdt_ptr)) {
            panic("xsdt src checksum failed");
        } else if(!acpi_sdt_checksum((acpi_sdt_header_t*)xsdt)) {
            panic("xsdt dst checksum failed");
        }
        clraccess_paddr_DANGER(xsdt_ptr);
    }
}

uint32_t find_table(char* sig) {
    if(acpi_version < 2) {
        uint32_t entries = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / 4;
        for(uint32_t i = 0; i < entries; i++) {
            acpi_sdt_header_t* entry = (acpi_sdt_header_t*)access_paddr_DANGER(rsdt->sdt_ptrs[i]);
            if(strncmp(entry->signature, sig, 4) == 0) {
                return (uint32_t)entry;
            }
            clraccess_paddr_DANGER(entry);
        }
    } else {
        uint32_t entries = (xsdt->header.length - sizeof(acpi_sdt_header_t)) / 8;
        for(uint32_t i = 0; i < entries; i++) {
            acpi_sdt_header_t* entry = (acpi_sdt_header_t*)access_paddr_DANGER(xsdt->sdt_ptrs[i]);
            if(strncmp(entry->signature, sig, 4) == 0) {
                return (uint32_t)entry;
            }
            clraccess_paddr_DANGER(entry);
        }
    }
    return 0;
}


void find_madt() {
    madt_ptr = (madt_t*)find_table(MADT_SIG);
    madt = kmalloc(madt_ptr->header.length);
    memcpy(madt, madt_ptr, madt_ptr->header.length);
    if(!acpi_sdt_checksum((acpi_sdt_header_t*)madt_ptr)) {
        panic("madt src checksum invalid");
    } else if(!acpi_sdt_checksum((acpi_sdt_header_t*)(madt))) {
        panic("madt dst checksum invalid");
    }
    clraccess_paddr_DANGER(madt_ptr);
}

void find_hpet() {
    hpet_ptr = (hpet_t*)find_table(HPET_SIG);
    hpet = kmalloc(hpet_ptr->header.length);
    memcpy(hpet, hpet_ptr, hpet_ptr->header.length);
    if(!acpi_sdt_checksum((acpi_sdt_header_t*)hpet_ptr)) {
        panic("hpet src checksum invalid");
    } else if(!acpi_sdt_checksum((acpi_sdt_header_t*)hpet)) {
        panic("hpet dst checksum invalid");
    }
    clraccess_paddr_DANGER(hpet_ptr);
}

void find_fadt() {
    fadt_ptr = (fadt_t*)find_table(FADT_SIG);
    fadt = kmalloc(sizeof(fadt_t));
    memcpy(fadt, fadt_ptr, fadt_ptr->header.length);
    if(!acpi_sdt_checksum((acpi_sdt_header_t*)fadt_ptr)) {
        panic("fadt src checksum invalid");
    } else if (!acpi_sdt_checksum((acpi_sdt_header_t*)fadt)) {
        panic("fadt dst checksum invalid");
    }
    clraccess_paddr_DANGER(fadt_ptr);
}

bool rsdp_checksum() {
    if(acpi_version < 2) {
        uint8_t sum = 0;
        uint8_t* byte = (uint8_t*)rsdp_ptr;
        for(uint8_t i = 0; i < 20; i++) {
            sum += *byte;
            byte++;
        }
        if(sum % 0x100 == 0) {
            return true;
        }
        return false;
    } else {
        uint8_t sum = 0;
        uint8_t* byte = (uint8_t*)rsdp_ptr;
        for(uint8_t i = 0; i < 36; i++) {
            sum += *byte;
            byte++;
        }
        if(sum % 0x100 == 0) {
            return true;
        }
        return false;
    }
}

bool acpi_sdt_checksum(acpi_sdt_header_t* header) {
    uint32_t sum = 0;
    uint8_t* byte = (uint8_t*)header;
    for(uint8_t i = 0; i < header->length; i++) {
        sum += *byte;
        byte++;
    }
    if(sum % 0x100 == 0) {
        return true;
    }
    return false;
}

int get_acpi_version() {
    return acpi_version;
}

rsdp_t* get_rsdp() {
    return rsdp;
}

rsdt_t* get_rsdt() {
    return rsdt;
}

xsdt_t* get_xsdt() {
    return xsdt;
}

madt_t* get_madt() {
    return madt;
}

hpet_t* get_hpet() {
    return hpet;
}

fadt_t* get_fadt() {
    return fadt;
}

void init_acpi() {
    void* fix = kmalloc(0x400000);
    printf("%x\n",(uint32_t)fix);
    kfree(fix);
    find_rsdp();
    find_rsdt_xsdt();
    find_madt();
    find_hpet();
    find_fadt();
    gdb_stop();
}