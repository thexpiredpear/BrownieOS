#ifndef _KERNEL_ACPI_H
#define _KERNEL_ACPI_H 1

#include <stdint.h>
#include <stdbool.h>

// 36 bytes
struct rsdp {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr;
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

typedef struct rsdp rsdp_t;

// 36 bytes
struct acpi_sdt_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    uint64_t oem_table_id;
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

typedef struct acpi_sdt_header acpi_sdt_header_t;

// 36 + 4 bytes per sdt
struct rsdt {
    acpi_sdt_header_t header;
    // each ptr 4 bytes, divide length of data portion by 4 for each ptr
    uint32_t sdt_ptrs[];
} __attribute__((packed));

typedef struct rsdt rsdt_t;

// 36 + 8 bytes per sdt
struct xsdt {
    acpi_sdt_header_t header;
    // each ptr 8 bytes, divide length of data portion by 8 for each ptr
    uint64_t sdt_ptrs[];
} __attribute__((packed));

typedef struct xsdt xsdt_t;

// 56 bytes
struct hpet {
    acpi_sdt_header_t header;
    uint8_t hardware_rev_id;
    uint8_t comparator_count:5;
    uint8_t counter_size:1;
    uint8_t reserved_bit:1;
    uint8_t legacy_replacement:1;
    uint16_t pci_vendor_id;
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t addr;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed));

typedef struct hpet hpet_t;


// 44 + 1 bytes per lapic
struct madt {
    acpi_sdt_header_t header;
    uint32_t lapic_addr;
    uint32_t flags;
    // each record variable length, have to use byte array
    // length of full table minus header minus 8 bytes for lapic addr and flags
    uint8_t records[];
} __attribute__((packed));

typedef struct madt madt_t;

struct fadt {
    acpi_sdt_header_t header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pm_tmr_len;
    uint8_t gpe0_blk_len;
    uint8_t gpe1_blk_len;
    uint8_t gpe1_base;
    uint8_t cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alrm;
    uint8_t mon_alrm;
    uint8_t century;
    uint16_t iapc_boot_arch;
    uint8_t reserved2;
    uint32_t flags;
    // 12 bytes of reserved
    uint8_t reset_reg[12];
    uint8_t reset_value;
    uint8_t reserved3[3];
    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;
    // 12 bytes of reserved
    uint8_t x_pm1a_evt_blk[12];
    uint8_t x_pm1b_evt_blk[12];
    uint8_t x_pm1a_cnt_blk[12];
    uint8_t x_pm1b_cnt_blk[12];
    uint8_t x_pm2_cnt_blk[12];
    uint8_t x_pm_tmr_blk[12];
    uint8_t x_gpe0_blk[12];
    uint8_t x_gpe1_blk[12];
} __attribute__((packed));

typedef struct fadt fadt_t;

// Locates the ACPI RSDP in physical memory (e.g., EBDA/BIOS areas), validates
// its checksum, and copies it into kernel memory for later access.
void find_rsdp(void);

// Maps and copies the RSDT (ACPI < 2.0) or XSDT (>= 2.0) into kernel memory,
// verifying the ACPI table checksum. Chooses structure based on RSDP revision.
void find_rsdt_xsdt(void);

// Searches the RSDT/XSDT for a table with the provided 4-character signature
// (e.g., "APIC", "HPET"). Returns a KERNEL virtual address of a mapped source
// table suitable for copying, or 0 if not found.
uint32_t find_table(char*);

// Verifies the RSDP checksum (20 bytes for v1.0, 36 for >= v2.0). Returns true
// if the sum of all bytes modulo 256 is 0.
bool rsdp_checksum();
// Verifies an ACPI SDT header’s checksum across the entire table as specified
// in the `length` field. `acpi_sdt_header_t*` is a kernel virtual pointer.
bool acpi_sdt_checksum(acpi_sdt_header_t*);

// Returns the detected ACPI version (e.g., 1 or 2+), derived from RSDP.
int get_acpi_version(void);

// Returns the kernel virtual pointer to the copied RSDP structure.
rsdp_t* get_rsdp(void);

// Returns the kernel virtual pointer to the copied RSDT (if ACPI < 2.0).
rsdt_t* get_rsdt(void);

// Returns the kernel virtual pointer to the copied XSDT (if ACPI >= 2.0).
xsdt_t* get_xsdt(void);

// Returns the kernel virtual pointer to the copied MADT (APIC) table.
madt_t* get_madt(void);

// Returns the kernel virtual pointer to the copied HPET table.
hpet_t* get_hpet(void);

// Returns the kernel virtual pointer to the copied FADT (FACP) table.
fadt_t* get_fadt(void);

// Top-level ACPI initialization: finds and validates RSDP, loads RSDT/XSDT,
// locates and copies key tables (MADT, HPET, FADT). Must run after paging.
void init_acpi(void);

#endif
