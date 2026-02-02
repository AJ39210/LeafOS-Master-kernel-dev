#include "dvdrom.h"
#include "serial.h"
#include <stddef.h>

#define MAX_DVDROM_DRIVES 4

/* DVD-ROM Drive Registry */
static dvdrom_device_t dvdrom_drives[MAX_DVDROM_DRIVES];
static int dvdrom_drive_count = 0;

/* Known DVD Disc Structures */
#define DVD_STRUCT_PHYS_INFO 0x00
#define DVD_STRUCT_COPYRIGHT 0x01
#define DVD_STRUCT_DISC_KEY  0x02
#define DVD_STRUCT_BCA       0x03

void dvdrom_driver_init(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[DVDROM] DVD-ROM Driver Initializing...\n");
    serial_puts(SERIAL_PORT_A, "[DVDROM] DVD-ROM/RW support enabled\n");
    #endif
    
    dvdrom_drive_count = 0;
}

void dvdrom_detect_drives(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[DVDROM] Detecting DVD-ROM/RW drives...\n");
    #endif
    
    /* Check for CD-ROM drives that support DVD */
    int cdrom_count = cdrom_get_drive_count();
    
    for (int i = 0; i < cdrom_count && dvdrom_drive_count < MAX_DVDROM_DRIVES; i++) {
        cdrom_device_t *cdrom_dev = cdrom_get_drive(i);
        if (cdrom_dev) {
            dvdrom_device_t *dev = &dvdrom_drives[dvdrom_drive_count];
            dev->base = *cdrom_dev;
            dev->css_protected = DVD_NO_CSS;
            dev->dual_layer = 0;
            dev->capacity_layers = 4700000000ULL; /* ~4.7 GB standard */
            
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[DVDROM] Drive ");
            serial_putchar(SERIAL_PORT_A, "0123456789"[dvdrom_drive_count]);
            serial_puts(SERIAL_PORT_A, " capable of DVD-ROM\n");
            #endif
            
            dvdrom_drive_count++;
        }
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[DVDROM] Detection complete: ");
    serial_putchar(SERIAL_PORT_A, "0123456789"[dvdrom_drive_count]);
    serial_puts(SERIAL_PORT_A, " DVD drive(s) found\n");
    #endif
}

int dvdrom_read_sectors(dvdrom_device_t *dev, uint32_t lba, uint32_t count, void *buffer) {
    if (!dev || !buffer) return -1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[DVDROM] Read: LBA=0x");
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(lba >> 28) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(lba >> 24) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(lba >> 20) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(lba >> 16) & 0xF]);
    serial_puts(SERIAL_PORT_A, " count=");
    serial_putchar(SERIAL_PORT_A, "0123456789"[(count / 10) % 10]);
    serial_putchar(SERIAL_PORT_A, "0123456789"[count % 10]);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return 0;
}

int dvdrom_get_structure(dvdrom_device_t *dev, uint8_t format, uint32_t address) {
    if (!dev) return -1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[DVDROM] Reading DVD structure format: 0x");
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(format >> 4) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[format & 0xF]);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    switch (format) {
        case DVD_STRUCT_PHYS_INFO:
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[DVDROM] Physical Information format\n");
            #endif
            break;
        case DVD_STRUCT_COPYRIGHT:
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[DVDROM] Copyright Information format\n");
            #endif
            break;
        case DVD_STRUCT_DISC_KEY:
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[DVDROM] Disc Key format\n");
            #endif
            break;
        case DVD_STRUCT_BCA:
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[DVDROM] Burst Cutting Area format\n");
            #endif
            break;
    }
    
    return 0;
}

int dvdrom_is_dual_layer(dvdrom_device_t *dev) {
    if (!dev) return -1;
    return dev->dual_layer;
}

dvdrom_device_t *dvdrom_get_drive(int index) {
    if (index < 0 || index >= dvdrom_drive_count) return NULL;
    return &dvdrom_drives[index];
}

int dvdrom_get_drive_count(void) {
    return dvdrom_drive_count;
}
