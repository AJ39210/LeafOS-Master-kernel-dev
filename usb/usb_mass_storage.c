#include "usb_mass_storage.h"
#include "serial.h"
#include <stddef.h>

#define MAX_STORAGE_DEVICES 4
static usb_mass_storage_device_t storage_devices[MAX_STORAGE_DEVICES];
static int storage_device_count = 0;

/* Known Mass Storage Devices */
static const struct {
    uint16_t vendor_id;
    uint16_t product_id;
    const char *name;
} known_storage_devices[] = {
    { 0x1D6B, 0x0101, "USB Stick" },
    { 0x0951, 0x1666, "Kingston DataTraveler" },
    { 0x0000, 0x0000, NULL }
};

void usb_mass_storage_driver_init(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB MSC] Mass Storage Driver Loaded\n");
    serial_puts(SERIAL_PORT_A, "[USB MSC] USB Stick/Drive support ready\n");
    #endif
    storage_device_count = 0;
}

void usb_mass_storage_probe_device(uint16_t vendor_id, uint16_t product_id) {
    if (storage_device_count >= MAX_STORAGE_DEVICES) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[USB MSC] Device limit reached\n");
        #endif
        return;
    }
    
    usb_mass_storage_device_t *dev = &storage_devices[storage_device_count];
    dev->vendor_id = vendor_id;
    dev->product_id = product_id;
    dev->address = 0;
    dev->capacity = 0;
    dev->block_size = 512;
    dev->lun = 0;
    
    const char *name = "Unknown USB Storage";
    for (int i = 0; known_storage_devices[i].name; i++) {
        if (known_storage_devices[i].vendor_id == vendor_id &&
            known_storage_devices[i].product_id == product_id) {
            name = known_storage_devices[i].name;
            break;
        }
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB MSC] Detected: ");
    serial_puts(SERIAL_PORT_A, name);
    serial_puts(SERIAL_PORT_A, " (Vendor=0x");
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(vendor_id >> 12) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(vendor_id >> 8) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(vendor_id >> 4) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[vendor_id & 0xF]);
    serial_puts(SERIAL_PORT_A, ")\n");
    #endif
    
    storage_device_count++;
}

int usb_storage_read_blocks(usb_mass_storage_device_t *dev, uint32_t block, uint32_t count, void *buffer) {
    if (!dev || !buffer) return -1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB MSC] Read: block=0x");
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(block >> 28) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(block >> 24) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(block >> 20) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(block >> 16) & 0xF]);
    serial_puts(SERIAL_PORT_A, " count=");
    serial_putchar(SERIAL_PORT_A, "0123456789"[count % 10]);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return 0;
}

int usb_storage_write_blocks(usb_mass_storage_device_t *dev, uint32_t block, uint32_t count, void *buffer) {
    if (!dev || !buffer) return -1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB MSC] Write: block=0x");
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(block >> 28) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(block >> 24) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(block >> 20) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(block >> 16) & 0xF]);
    serial_puts(SERIAL_PORT_A, " count=");
    serial_putchar(SERIAL_PORT_A, "0123456789"[count % 10]);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return 0;
}

int usb_mass_storage_get_device_count(void) {
    return storage_device_count;
}

usb_mass_storage_device_t *usb_mass_storage_get_device(int index) {
    if (index < 0 || index >= storage_device_count) return NULL;
    return &storage_devices[index];
}
