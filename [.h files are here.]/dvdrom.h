#ifndef DVDROM_H
#define DVDROM_H

#include "cdrom.h"

/* DVD-ROM Disc Types */
#define DVDROM_TYPE_DVDROM    0x10  /* DVD-ROM */
#define DVDROM_TYPE_DVDRW     0x11  /* DVD-R/RW */
#define DVDROM_TYPE_DVDP      0x12  /* DVD+R/RW */
#define DVDROM_TYPE_UNKNOWN   0xFF

/* DVD-Specific ATAPI Commands */
#define ATAPI_CMD_READ_DVD_STRUCTURE  0xAD
#define ATAPI_CMD_REPORT_KEY          0xA4
#define ATAPI_CMD_SEND_KEY            0xA3
#define ATAPI_CMD_READ_DVDRAM_INFO    0x4C

/* DVD Sector Size */
#define DVDROM_SECTOR_SIZE 2048

/* DVD Protection/CSS */
#define DVD_NO_CSS 0
#define DVD_CSS_PROTECTED 1

typedef struct {
    cdrom_device_t base;
    uint8_t css_protected;
    uint64_t capacity_layers;
    uint8_t dual_layer;
} dvdrom_device_t;

void dvdrom_driver_init(void);
void dvdrom_detect_drives(void);
int dvdrom_read_sectors(dvdrom_device_t *dev, uint32_t lba, uint32_t count, void *buffer);
int dvdrom_get_structure(dvdrom_device_t *dev, uint8_t format, uint32_t address);
int dvdrom_is_dual_layer(dvdrom_device_t *dev);
dvdrom_device_t *dvdrom_get_drive(int index);
int dvdrom_get_drive_count(void);

#endif
