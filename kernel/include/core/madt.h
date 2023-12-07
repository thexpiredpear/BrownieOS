#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <core/acpi.h>

struct record_header {
    uint8_t type;
    uint8_t length;
} __attribute((packed));

typedef struct record_header record_header_t;

struct type_0_lapic {
    record_header_t header;
    uint8_t acpi_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute((packed));

typedef struct type_0_lapic lapic_t;

struct type_1_ioapic {
    record_header_t header;
    uint8_t id;
    uint8_t reserved;
    uint32_t addr;
    uint32_t gsi_base;
} __attribute((packed));

typedef struct type_1_ioapic ioapic_t;

struct type_2_ioapic_iso {
    record_header_t header;
    uint8_t bus_src;
    uint8_t irq_src;
    uint32_t gsi;
    uint16_t flags;
} __attribute((packed));

typedef struct type_2_ioapic_iso ioapic_iso_t;

struct type_3_nmi_src {
    record_header_t header;
    uint8_t nmi_src;
    uint8_t reserved;
    uint16_t flags;
    uint32_t gsi;
} __attribute((packed));

typedef struct type_3_nmi_src nmi_src_t;

struct type_4_lapic_nmi {
    record_header_t header;
    uint8_t acpi_id;
    uint16_t flags;
    uint8_t lint;
} __attribute((packed));

typedef struct type_4_lapic_nmi lapic_nmi_t;

struct type_5_lapic_addr_ovr {
    record_header_t header;
    uint16_t reserved;
    uint64_t addr;
} __attribute((packed));

typedef struct type_5_lapic_addr_ovr lapic_addr_ovr_t;

struct type_9_lapic_x2apic {
    record_header_t header;
    uint16_t reserved;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute((packed));

typedef struct type_9_lapic_x2apic lapic_x2apic_t;

enum {
    LAPIC = 0,
    IOAPIC = 1,
    IOAPIC_ISO = 2,
    NMI_SRC = 3,
    LAPIC_NMI = 4,
    LAPIC_ADDR_OVR = 5,
    LAPIC_X2APIC = 9
};

void parse_madt();

uint8_t get_num_procs();
uint8_t* get_lapic_ids();
uint32_t get_lapic_addr();
uint32_t get_ioapic_addr();
ioapic_iso_t* get_iso_entries();