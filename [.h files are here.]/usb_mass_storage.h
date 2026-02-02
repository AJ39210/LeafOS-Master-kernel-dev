#ifndef USB_MASS_STORAGE_H
#define USB_MASS_STORAGE_H

#include <stdint.h>

/* USB Mass Storage Class */
#define USB_MSC_SUBCLASS_SCSI    0x06
#define USB_MSC_PROTOCOL_BBB     0x50

/* SCSI Commands */
#define SCSI_CMD_TEST_UNIT_READY 0x00
#define SCSI_CMD_READ_CAPACITY   0x25
#define SCSI_CMD_READ_10         0x28
#define SCSI_CMD_WRITE_10        0x2A

typedef struct {
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t address;
    uint32_t capacity;
    uint32_t block_size;
    uint8_t lun;
} usb_mass_storage_device_t;

void usb_mass_storage_driver_init(void);
void usb_mass_storage_probe_device(uint16_t vendor_id, uint16_t product_id);
int usb_storage_read_blocks(usb_mass_storage_device_t *dev, uint32_t block, uint32_t count, void *buffer);
int usb_storage_write_blocks(usb_mass_storage_device_t *dev, uint32_t block, uint32_t count, void *buffer);

#endif
