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

void find_rsdp(void);

void find_rsdt_xsdt(void);

uint32_t find_table(char*);

bool rsdp_checksum();
bool acpi_sdt_checksum(acpi_sdt_header_t*);

int get_acpi_version(void);

rsdp_t* get_rsdp(void);

rsdt_t* get_rsdt(void);

xsdt_t* get_xsdt(void);

madt_t* get_madt(void);

hpet_t* get_hpet(void);

fadt_t* get_fadt(void);

void init_acpi(void);

#endif