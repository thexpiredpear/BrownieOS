#ifndef _KERNEL_ATA_H
#define _KERNEL_ATA_H

#include <stdint.h>
#include <stdbool.h>

#define ATA_SECTOR_SIZE 512

enum {
    ATA_OK = 0,
    ATA_E_TIMEOUT = -1,
    ATA_E_NODEV = -2,
    ATA_E_RANGE = -3,
    ATA_E_IO = -4,
    ATA_E_UNSUP = -5,
};

typedef struct {
    bool present;
    bool atapi;
    bool lba48;
    uint32_t lba28_sectors;
    uint64_t lba48_sectors;
    char model[41];
} ata_device_t;

void ata_init(void);
bool ata_present(void);
const ata_device_t* ata_device0(void);

int ata_pio_read28(uint32_t lba, uint8_t count, void* out);
int ata_pio_write28(uint32_t lba, uint8_t count, const void* in);
int ata_flush_cache(void);

#endif
