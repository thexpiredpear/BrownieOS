#include <drivers/ata.h>
#include <core/common.h>
#include <stdio.h>
#include <string.h>

#define ATA_PRIMARY_IO   0x1F0
#define ATA_PRIMARY_CTRL 0x3F6

#define ATA_REG_DATA       0
#define ATA_REG_ERROR      1
#define ATA_REG_FEATURES   1
#define ATA_REG_SECCOUNT0  2
#define ATA_REG_LBA0       3
#define ATA_REG_LBA1       4
#define ATA_REG_LBA2       5
#define ATA_REG_HDDEVSEL   6
#define ATA_REG_STATUS     7
#define ATA_REG_COMMAND    7

#define ATA_SR_BSY  0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DRQ  0x08
#define ATA_SR_ERR  0x01

#define ATA_CMD_IDENTIFY     0xEC
#define ATA_CMD_READ_PIO     0x20
#define ATA_CMD_WRITE_PIO    0x30
#define ATA_CMD_CACHE_FLUSH  0xE7

#define ATA_SPIN_LIMIT 1000000

static ata_device_t ata0;
static bool ata_inited = false;

static inline uint8_t ata_status(void) {
    return inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
}

static inline uint8_t ata_altstatus(void) {
    return inb(ATA_PRIMARY_CTRL);
}

static void ata_io_wait_400ns(void) {
    // ide needs a tiny delay after drive select
    ata_altstatus();
    ata_altstatus();
    ata_altstatus();
    ata_altstatus();
}

static int ata_wait_not_bsy(void) {
    for (uint32_t i = 0; i < ATA_SPIN_LIMIT; ++i) {
        if (!(ata_status() & ATA_SR_BSY)) {
            return ATA_OK;
        }
    }
    return ATA_E_TIMEOUT;
}

static int ata_wait_drq_or_err(void) {
    for (uint32_t i = 0; i < ATA_SPIN_LIMIT; ++i) {
        uint8_t st = ata_status();
        if (st & ATA_SR_ERR) {
            return ATA_E_IO;
        }
        if (st & ATA_SR_DRQ) {
            return ATA_OK;
        }
    }
    return ATA_E_TIMEOUT;
}

static void ata_select(bool slave, uint32_t lba) {
    uint8_t head = 0xE0 | (slave ? 0x10 : 0x00) | ((lba >> 24) & 0x0F);
    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, head);
    ata_io_wait_400ns();
}

static void ata_parse_model(char* dst, const uint16_t* id) {
    // model string is word-swapped
    for (int i = 0; i < 20; ++i) {
        uint16_t w = id[27 + i];
        dst[i * 2] = (char)(w >> 8);
        dst[i * 2 + 1] = (char)(w & 0xFF);
    }
    dst[40] = '\0';
    for (int i = 39; i >= 0; --i) {
        if (dst[i] == ' ' || dst[i] == '\0') {
            dst[i] = '\0';
        } else {
            break;
        }
    }
}

static int ata_identify(bool slave, ata_device_t* dev) {
    uint16_t id[256];

    memset(dev, 0, sizeof(*dev));
    ata_select(slave, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    if (ata_status() == 0) {
        return ATA_E_NODEV;
    }

    if (ata_wait_not_bsy() != ATA_OK) {
        return ATA_E_TIMEOUT;
    }

    uint8_t lba1 = inb(ATA_PRIMARY_IO + ATA_REG_LBA1);
    uint8_t lba2 = inb(ATA_PRIMARY_IO + ATA_REG_LBA2);
    if (lba1 != 0 || lba2 != 0) {
        dev->atapi = true;
        return ATA_E_UNSUP;
    }

    int r = ata_wait_drq_or_err();
    if (r != ATA_OK) {
        return r;
    }

    insw(ATA_PRIMARY_IO + ATA_REG_DATA, id, 256);

    ata_parse_model(dev->model, id);
    dev->lba28_sectors = ((uint32_t)id[61] << 16) | id[60];
    dev->lba48 = (id[83] & (1 << 10)) != 0;
    if (dev->lba48) {
        dev->lba48_sectors =
            ((uint64_t)id[103] << 48) |
            ((uint64_t)id[102] << 32) |
            ((uint64_t)id[101] << 16) |
            ((uint64_t)id[100]);
    }
    dev->present = true;
    return ATA_OK;
}

void ata_init(void) {
    if (ata_inited) {
        return;
    }
    ata_inited = true;

    outb(ATA_PRIMARY_CTRL, 0x02); // disable irq from ide for polling mode

    int r = ata_identify(false, &ata0);
    if (r == ATA_OK) {
        printf("ata: %s, lba28 %u", ata0.model, ata0.lba28_sectors);
        if (ata0.lba48) {
            printf(", lba48 %u\n", (uint32_t)ata0.lba48_sectors);
        } else {
            printf("\n");
        }
        return;
    }

    if (r == ATA_E_UNSUP && ata0.atapi) {
        printf("ata: atapi device on primary master (unsupported)\n");
    } else {
        printf("ata: no disk on primary master\n");
    }
}

bool ata_present(void) {
    return ata0.present;
}

const ata_device_t* ata_device0(void) {
    return &ata0;
}

int ata_pio_read28(uint32_t lba, uint8_t count, void* out) {
    if (!ata0.present || ata0.atapi) {
        return ATA_E_NODEV;
    }
    if (count == 0 || out == NULL) {
        return ATA_E_RANGE;
    }
    if (lba > 0x0FFFFFFF) {
        return ATA_E_RANGE;
    }
    if (ata0.lba28_sectors != 0 &&
        (lba + (uint32_t)count) > ata0.lba28_sectors) {
        return ATA_E_RANGE;
    }

    if (ata_wait_not_bsy() != ATA_OK) {
        return ATA_E_TIMEOUT;
    }

    ata_select(false, lba);
    outb(ATA_PRIMARY_IO + ATA_REG_FEATURES, 0x00);
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, count);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    uint8_t* dst = (uint8_t*)out;
    for (uint32_t i = 0; i < count; ++i) {
        int r = ata_wait_drq_or_err();
        if (r != ATA_OK) {
            return r;
        }
        insw(ATA_PRIMARY_IO + ATA_REG_DATA, dst, 256);
        dst += ATA_SECTOR_SIZE;
    }
    return ATA_OK;
}

int ata_pio_write28(uint32_t lba, uint8_t count, const void* in) {
    if (!ata0.present || ata0.atapi) {
        return ATA_E_NODEV;
    }
    if (count == 0 || in == NULL) {
        return ATA_E_RANGE;
    }
    if (lba > 0x0FFFFFFF) {
        return ATA_E_RANGE;
    }
    if (ata0.lba28_sectors != 0 &&
        (lba + (uint32_t)count) > ata0.lba28_sectors) {
        return ATA_E_RANGE;
    }

    if (ata_wait_not_bsy() != ATA_OK) {
        return ATA_E_TIMEOUT;
    }

    ata_select(false, lba);
    outb(ATA_PRIMARY_IO + ATA_REG_FEATURES, 0x00);
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, count);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    const uint8_t* src = (const uint8_t*)in;
    for (uint32_t i = 0; i < count; ++i) {
        int r = ata_wait_drq_or_err();
        if (r != ATA_OK) {
            return r;
        }
        outsw(ATA_PRIMARY_IO + ATA_REG_DATA, src, 256);
        src += ATA_SECTOR_SIZE;
    }

    return ata_flush_cache();
}

int ata_flush_cache(void) {
    if (!ata0.present || ata0.atapi) {
        return ATA_E_NODEV;
    }
    if (ata_wait_not_bsy() != ATA_OK) {
        return ATA_E_TIMEOUT;
    }
    ata_select(false, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    if (ata_wait_not_bsy() != ATA_OK) {
        return ATA_E_TIMEOUT;
    }
    if (ata_status() & ATA_SR_ERR) {
        return ATA_E_IO;
    }
    return ATA_OK;
}
