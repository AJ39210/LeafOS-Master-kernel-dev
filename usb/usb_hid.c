#include "usb_hid.h"
#include "serial.h"
#include <stddef.h>

#define MAX_HID_DEVICES 8
static usb_hid_device_t hid_devices[MAX_HID_DEVICES];
static int hid_device_count = 0;

/* Known HID Devices */
static const struct {
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t type;
    const char *name;
} known_hid_devices[] = {
    { 0x1D6B, 0x0101, 0, "Linux Keyboard" },
    { 0x1D6B, 0x0102, 1, "Linux Mouse" },
    { 0x0000, 0x0000, 0, NULL }
};

void usb_hid_driver_init(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB HID] HID Driver Loaded\n");
    serial_puts(SERIAL_PORT_A, "[USB HID] Keyboard/Mouse driver ready\n");
    #endif
    hid_device_count = 0;
}

void usb_hid_probe_device(uint16_t vendor_id, uint16_t product_id, uint8_t class) {
    if (class != 0x03) return;
    
    if (hid_device_count >= MAX_HID_DEVICES) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[USB HID] Device limit reached\n");
        #endif
        return;
    }
    
    usb_hid_device_t *dev = &hid_devices[hid_device_count];
    dev->vendor_id = vendor_id;
    dev->product_id = product_id;
    dev->address = 0;
    dev->report_id = 0;
    dev->report_size = 8;
    
    /* Detect keyboard vs mouse */
    for (int i = 0; known_hid_devices[i].name; i++) {
        if (known_hid_devices[i].vendor_id == vendor_id &&
            known_hid_devices[i].product_id == product_id) {
            dev->type = known_hid_devices[i].type;
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[USB HID] Detected: ");
            serial_puts(SERIAL_PORT_A, known_hid_devices[i].name);
            serial_puts(SERIAL_PORT_A, "\n");
            #endif
            hid_device_count++;
            return;
        }
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB HID] Unknown HID device\n");
    #endif
}

int usb_hid_get_device_count(void) {
    return hid_device_count;
}

usb_hid_device_t *usb_hid_get_device(int index) {
    if (index < 0 || index >= hid_device_count) return NULL;
    return &hid_devices[index];
}
