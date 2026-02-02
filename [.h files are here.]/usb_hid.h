#ifndef USB_HID_H
#define USB_HID_H

#include <stdint.h>

/* HID Report Types */
#define HID_REPORT_INPUT   0x01
#define HID_REPORT_OUTPUT  0x02
#define HID_REPORT_FEATURE 0x03

/* HID Usage Pages */
#define HID_USAGE_PAGE_GENERIC  0x01
#define HID_USAGE_PAGE_KEYBOARD 0x07
#define HID_USAGE_PAGE_LED      0x08
#define HID_USAGE_PAGE_BUTTON   0x09

/* HID Usages */
#define HID_USAGE_KEYBOARD 0x06
#define HID_USAGE_MOUSE    0x02

typedef struct {
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t type; /* 0=keyboard, 1=mouse */
    uint8_t address;
    uint8_t report_id;
    uint8_t report_size;
} usb_hid_device_t;

void usb_hid_driver_init(void);
void usb_hid_probe_device(uint16_t vendor_id, uint16_t product_id, uint8_t class);

#endif
