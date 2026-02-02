#include "cdrom.h"
#include "serial.h"
#include <stddef.h>

#define MAX_CDROM_DRIVES 4

/* CD-ROM Drive Registry */
static cdrom_device_t cdrom_drives[MAX_CDROM_DRIVES];
static int cdrom_drive_count = 0;

/* ATA Command Port Access */
#define ATA_PRIMARY_BASE   0x1F0
#define ATA_SECONDARY_BASE 0x170

#define ATA_DATA_REG        0x00
#define ATA_ERROR_REG       0x01
#define ATA_SECTOR_COUNT    0x02
#define ATA_LBA_LOW         0x03
#define ATA_LBA_MID         0x04
#define ATA_LBA_HIGH        0x05
#define ATA_DEVICE_REG      0x06
#define ATA_STATUS_REG      0x07
#define ATA_COMMAND_REG     0x07

#define ATA_CMD_IDENTIFY_PACKET_DEVICE 0xA1
#define ATA_CMD_PACKET 0xA0

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static void atapi_send_command(uint16_t ata_base, uint8_t *cmd, int cmd_len, void *buffer, uint32_t buffer_len) {
    /* Set up ATAPI packet command */
    outb(ata_base + ATA_SECTOR_COUNT, 0);
    outb(ata_base + ATA_LBA_MID, (buffer_len) & 0xFF);
    outb(ata_base + ATA_LBA_HIGH, (buffer_len >> 8) & 0xFF);
    
    /* Issue PACKET command */
    outb(ata_base + ATA_COMMAND_REG, ATA_CMD_PACKET);
    
    /* Send 12-byte ATAPI command */
    for (int i = 0; i < cmd_len; i += 2) {
        uint16_t word = cmd[i] | (cmd[i + 1] << 8);
        outw(ata_base + ATA_DATA_REG, word);
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[CDROM] ATAPI command sent\n");
    #endif
}

void cdrom_driver_init(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[CDROM] CD-ROM Driver Initializing...\n");
    serial_puts(SERIAL_PORT_A, "[CDROM] Scanning primary and secondary ATA channels\n");
    #endif
    
    cdrom_drive_count = 0;
}

void cdrom_detect_drives(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[CDROM] Detecting CD-ROM/DVDROM drives...\n");
    #endif
    
    /* Check primary ATA channel */
    uint8_t device_reg = inb(ATA_PRIMARY_BASE + ATA_DEVICE_REG);
    uint8_t status = inb(ATA_PRIMARY_BASE + ATA_STATUS_REG);
    
    if (status != 0xFF && status != 0x00) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[CDROM] Found device on primary ATA channel\n");
        serial_puts(SERIAL_PORT_A, "[CDROM] Status: 0x");
        serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(status >> 4) & 0xF]);
        serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[status & 0xF]);
        serial_puts(SERIAL_PORT_A, "\n");
        #endif
        
        if (cdrom_drive_count < MAX_CDROM_DRIVES) {
            cdrom_device_t *dev = &cdrom_drives[cdrom_drive_count];
            dev->state = CDROM_STATE_EMPTY;
            dev->address = 0xA0;
            dev->disc_type = CDROM_TYPE_UNKNOWN;
            dev->sector_size = CDROM_SECTOR_SIZE_2048;
            dev->capacity_sectors = 0;
            dev->capacity_bytes = 0;
            
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[CDROM] CD-ROM Drive 0 registered\n");
            #endif
            cdrom_drive_count++;
        }
    }
    
    /* Check secondary ATA channel */
    device_reg = inb(ATA_SECONDARY_BASE + ATA_DEVICE_REG);
    status = inb(ATA_SECONDARY_BASE + ATA_STATUS_REG);
    
    if (status != 0xFF && status != 0x00) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[CDROM] Found device on secondary ATA channel\n");
        #endif
        
        if (cdrom_drive_count < MAX_CDROM_DRIVES) {
            cdrom_device_t *dev = &cdrom_drives[cdrom_drive_count];
            dev->state = CDROM_STATE_EMPTY;
            dev->address = 0xB0;
            dev->disc_type = CDROM_TYPE_UNKNOWN;
            dev->sector_size = CDROM_SECTOR_SIZE_2048;
            dev->capacity_sectors = 0;
            dev->capacity_bytes = 0;
            
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[CDROM] CD-ROM Drive 1 registered\n");
            #endif
            cdrom_drive_count++;
        }
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[CDROM] Detection complete: ");
    serial_putchar(SERIAL_PORT_A, "0123456789"[cdrom_drive_count]);
    serial_puts(SERIAL_PORT_A, " drive(s) found\n");
    #endif
}

int cdrom_read_sectors(cdrom_device_t *dev, uint32_t lba, uint32_t count, void *buffer) {
    if (!dev || !buffer) return -1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[CDROM] Read: LBA=0x");
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

int cdrom_read_toc(cdrom_device_t *dev, cd_toc_entry_t *toc, int max_entries) {
    if (!dev || !toc) return -1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[CDROM] Reading Table of Contents\n");
    #endif
    
    return 0;
}

int cdrom_eject(cdrom_device_t *dev) {
    if (!dev) return -1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[CDROM] Ejecting disc\n");
    #endif
    
    return 0;
}

int cdrom_get_disc_type(cdrom_device_t *dev) {
    if (!dev) return -1;
    return dev->disc_type;
}

cdrom_device_t *cdrom_get_drive(int index) {
    if (index < 0 || index >= cdrom_drive_count) return NULL;
    return &cdrom_drives[index];
}

int cdrom_get_drive_count(void) {
    return cdrom_drive_count;
}
