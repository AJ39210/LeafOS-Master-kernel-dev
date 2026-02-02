/**
 * drivers/ata/ata.h
 * 
 * LeafOS ATA (Advanced Technology Attachment) Driver
 * 
 * Provides support for IDE/ATA disk drives
 * Supports PIO (Programmed I/O) mode for basic operations
 */

#ifndef ATA_H
#define ATA_H

#include <stdint.h>

/* ATA Controller Registers */
#define ATA_PRIMARY_BASE    0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_SECONDARY_BASE  0x170
#define ATA_SECONDARY_CTRL  0x376

/* ATA Register Offsets */
#define ATA_REG_DATA        0x00
#define ATA_REG_ERROR       0x01
#define ATA_REG_FEATURES    0x01
#define ATA_REG_SECCOUNT    0x02
#define ATA_REG_LBALO       0x03
#define ATA_REG_LBAMID      0x04
#define ATA_REG_LBAHI       0x05
#define ATA_REG_DEVICE      0x06
#define ATA_REG_STATUS      0x07
#define ATA_REG_COMMAND     0x07

/* ATA Commands */
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_WRITE_PIO   0x30
#define ATA_CMD_IDENTIFY    0xEC
#define ATA_CMD_FLUSH_CACHE 0xE7

/* ATA Status Flags */
#define ATA_STATUS_BUSY     0x80
#define ATA_STATUS_DRDY     0x40
#define ATA_STATUS_DRQ      0x08
#define ATA_STATUS_ERR      0x01

/* ATA Error Flags */
#define ATA_ERROR_AMNF      0x01
#define ATA_ERROR_TKZNF     0x02
#define ATA_ERROR_ABRT      0x04
#define ATA_ERROR_MCR       0x08

/* Device Types */
typedef enum {
    ATA_DEVICE_NONE = 0,
    ATA_DEVICE_PATA = 1,    /* Parallel ATA (IDE) */
    ATA_DEVICE_SATA = 2
} ata_device_type_t;

/* ATA Channel */
typedef enum {
    ATA_CHANNEL_PRIMARY = 0,
    ATA_CHANNEL_SECONDARY = 1
} ata_channel_t;

/* ATA Device Info */
typedef struct {
    uint16_t base_port;
    uint16_t ctrl_port;
    ata_device_type_t type;
    uint8_t drive_select;
    uint32_t sectors;
    uint16_t cylinders;
    uint16_t heads;
    uint16_t spt;
    char model[41];
    char serial[21];
} ata_device_t;

/* Global ATA State */
typedef struct {
    ata_device_t primary_master;
    ata_device_t primary_slave;
    ata_device_t secondary_master;
    ata_device_t secondary_slave;
    int initialized;
    int devices_found;
} ata_state_t;

/* Core Functions */
int ata_init(void);
int ata_detect_devices(void);
int ata_identify_device(ata_channel_t channel, uint8_t drive);

/* I/O Functions */
int ata_read_sectors(ata_channel_t channel, uint8_t drive, uint32_t lba, uint16_t count, uint8_t *buffer);
int ata_write_sectors(ata_channel_t channel, uint8_t drive, uint32_t lba, uint16_t count, uint8_t *buffer);

/* Control Functions */
int ata_soft_reset(ata_channel_t channel);
int ata_flush_cache(ata_channel_t channel, uint8_t drive);

/* Utility Functions */
uint8_t ata_read_status(ata_channel_t channel);
void ata_wait_busy(ata_channel_t channel);
void ata_wait_drq(ata_channel_t channel);

#endif // ATA_H
