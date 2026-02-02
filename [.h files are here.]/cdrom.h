#ifndef CDROM_H
#define CDROM_H

#include <stdint.h>

/* CD-ROM Device States */
#define CDROM_STATE_EMPTY    0
#define CDROM_STATE_LOADING  1
#define CDROM_STATE_READY    2
#define CDROM_STATE_READING  3
#define CDROM_STATE_ERROR    4

/* ATAPI Commands for CD-ROM */
#define ATAPI_CMD_TEST_UNIT_READY       0x00
#define ATAPI_CMD_REQUEST_SENSE         0x03
#define ATAPI_CMD_INQUIRY               0x12
#define ATAPI_CMD_READ_CAPACITY         0x25
#define ATAPI_CMD_READ_10               0x28
#define ATAPI_CMD_SEEK                  0x2B
#define ATAPI_CMD_READ_TOC              0x43
#define ATAPI_CMD_START_STOP_UNIT       0x1B
#define ATAPI_CMD_MODE_SELECT           0x55
#define ATAPI_CMD_MODE_SENSE            0x5A
#define ATAPI_CMD_PREVENT_ALLOW_REMOVAL 0x1E

/* CD-ROM Disc Types */
#define CDROM_TYPE_CDDA      0x00  /* Audio CD */
#define CDROM_TYPE_CDROM     0x01  /* Data CD */
#define CDROM_TYPE_CDROMXA   0x02  /* CD-ROM XA */
#define CDROM_TYPE_UNKNOWN   0xFF

/* CD-ROM Sector Sizes */
#define CDROM_SECTOR_SIZE_2048 2048
#define CDROM_SECTOR_SIZE_2336 2336
#define CDROM_SECTOR_SIZE_2352 2352

/* CD-ROM Addresses */
#define CD_MSF_MASK 0x00FFFFFF
#define CD_LBA_MASK 0xFFFFFFFF

typedef struct {
    uint8_t disc_type;
    uint32_t capacity_sectors;
    uint32_t capacity_bytes;
    uint16_t sector_size;
    uint8_t state;
    uint8_t address;
} cdrom_device_t;

typedef struct {
    uint8_t minute;
    uint8_t second;
    uint8_t frame;
} __attribute__((packed)) cd_msf_t;

typedef struct {
    uint32_t track_num;
    uint32_t start_lba;
    uint8_t adr;
    uint8_t control;
} cd_toc_entry_t;

void cdrom_driver_init(void);
void cdrom_detect_drives(void);
int cdrom_read_sectors(cdrom_device_t *dev, uint32_t lba, uint32_t count, void *buffer);
int cdrom_read_toc(cdrom_device_t *dev, cd_toc_entry_t *toc, int max_entries);
int cdrom_eject(cdrom_device_t *dev);
int cdrom_get_disc_type(cdrom_device_t *dev);
cdrom_device_t *cdrom_get_drive(int index);
int cdrom_get_drive_count(void);

#endif
