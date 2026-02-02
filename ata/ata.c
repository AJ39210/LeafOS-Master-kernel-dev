/**
 * drivers/ata/ata.c
 * 
 * LeafOS ATA Driver Implementation
 * 
 * Supports IDE/ATA parallel disk drives with PIO mode
 * Handles device detection, identification, and basic I/O
 */

#include "ata.h"
#include "../../include/serial.h"
#include <stddef.h>

static ata_state_t ata_state = {0};

/**
 * ata_read_status()
 * Read status register from ATA channel
 */
uint8_t ata_read_status(ata_channel_t channel)
{
    uint16_t base = (channel == ATA_CHANNEL_PRIMARY) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
    return *(volatile uint8_t *)(base + ATA_REG_STATUS);
}

/**
 * ata_wait_busy()
 * Wait until device is not busy
 */
void ata_wait_busy(ata_channel_t channel)
{
    uint8_t status;
    int timeout = 10000;
    
    do {
        status = ata_read_status(channel);
        timeout--;
    } while ((status & ATA_STATUS_BUSY) && timeout > 0);
}

/**
 * ata_wait_drq()
 * Wait for data ready
 */
void ata_wait_drq(ata_channel_t channel)
{
    uint8_t status;
    int timeout = 10000;
    
    do {
        status = ata_read_status(channel);
        timeout--;
    } while (!(status & ATA_STATUS_DRQ) && timeout > 0);
}

/**
 * ata_soft_reset()
 * Perform soft reset on channel
 */
int ata_soft_reset(ata_channel_t channel)
{
    uint16_t ctrl_port = (channel == ATA_CHANNEL_PRIMARY) ? ATA_PRIMARY_CTRL : ATA_SECONDARY_CTRL;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ATA] Soft reset on channel ");
    serial_putchar(SERIAL_PORT_A, '0' + channel);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    // Assert reset
    *(volatile uint8_t *)(ctrl_port) = 0x04;
    
    // Delay
    for (volatile int i = 0; i < 1000; i++);
    
    // Release reset
    *(volatile uint8_t *)(ctrl_port) = 0x00;
    
    // Wait for device
    ata_wait_busy(channel);
    
    return 1;
}

/**
 * ata_identify_device()
 * Identify ATA device on channel
 */
int ata_identify_device(ata_channel_t channel, uint8_t drive)
{
    uint16_t base = (channel == ATA_CHANNEL_PRIMARY) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ATA] Identifying device on channel ");
    serial_putchar(SERIAL_PORT_A, '0' + channel);
    serial_puts(SERIAL_PORT_A, ", drive ");
    serial_putchar(SERIAL_PORT_A, '0' + drive);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    // Select drive
    *(volatile uint8_t *)(base + ATA_REG_DEVICE) = (drive == 0) ? 0xA0 : 0xB0;
    
    // Wait for ready
    for (volatile int i = 0; i < 1000; i++);
    
    // Send IDENTIFY command
    *(volatile uint8_t *)(base + ATA_REG_COMMAND) = ATA_CMD_IDENTIFY;
    
    // Check status
    uint8_t status = ata_read_status(channel);
    if (status == 0) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[ATA] No device found\n");
        #endif
        return 0;
    }
    
    // Wait for data ready
    ata_wait_drq(channel);
    
    // Read identification data (256 words = 512 bytes)
    uint16_t *identify_data = (uint16_t *)(base + ATA_REG_DATA);
    for (int i = 0; i < 256; i++) {
        volatile uint16_t word = identify_data[0];
        (void)word;  // Use word to avoid unused variable warning
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ATA] Device identified successfully\n");
    #endif
    
    return 1;
}

/**
 * ata_read_sectors()
 * Read sectors from ATA device
 */
int ata_read_sectors(ata_channel_t channel, uint8_t drive, uint32_t lba, uint16_t count, uint8_t *buffer)
{
    if (!buffer || count == 0) {
        return 0;
    }
    
    uint16_t base = (channel == ATA_CHANNEL_PRIMARY) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ATA] Read ");
    #endif
    
    // Select drive
    *(volatile uint8_t *)(base + ATA_REG_DEVICE) = (drive == 0) ? 0xE0 : 0xF0;
    *(volatile uint8_t *)(base + ATA_REG_DEVICE) |= (uint8_t)((lba >> 24) & 0x0F);
    
    // Set sector count
    *(volatile uint8_t *)(base + ATA_REG_SECCOUNT) = (uint8_t)count;
    
    // Set LBA
    *(volatile uint8_t *)(base + ATA_REG_LBALO) = (uint8_t)lba;
    *(volatile uint8_t *)(base + ATA_REG_LBAMID) = (uint8_t)(lba >> 8);
    *(volatile uint8_t *)(base + ATA_REG_LBAHI) = (uint8_t)(lba >> 16);
    
    // Send read command
    *(volatile uint8_t *)(base + ATA_REG_COMMAND) = ATA_CMD_READ_PIO;
    
    // Read sectors
    for (uint16_t i = 0; i < count; i++) {
        ata_wait_drq(channel);
        
        // Read 512 bytes (256 words)
        uint16_t *data_port = (uint16_t *)(base + ATA_REG_DATA);
        for (int j = 0; j < 256; j++) {
            uint16_t word = *data_port;
            buffer[i * 512 + j * 2] = (uint8_t)word;
            buffer[i * 512 + j * 2 + 1] = (uint8_t)(word >> 8);
        }
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "sectors OK\n");
    #endif
    
    return 1;
}

/**
 * ata_write_sectors()
 * Write sectors to ATA device
 */
int ata_write_sectors(ata_channel_t channel, uint8_t drive, uint32_t lba, uint16_t count, uint8_t *buffer)
{
    if (!buffer || count == 0) {
        return 0;
    }
    
    uint16_t base = (channel == ATA_CHANNEL_PRIMARY) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ATA] Write ");
    #endif
    
    // Select drive
    *(volatile uint8_t *)(base + ATA_REG_DEVICE) = (drive == 0) ? 0xE0 : 0xF0;
    *(volatile uint8_t *)(base + ATA_REG_DEVICE) |= (uint8_t)((lba >> 24) & 0x0F);
    
    // Set sector count
    *(volatile uint8_t *)(base + ATA_REG_SECCOUNT) = (uint8_t)count;
    
    // Set LBA
    *(volatile uint8_t *)(base + ATA_REG_LBALO) = (uint8_t)lba;
    *(volatile uint8_t *)(base + ATA_REG_LBAMID) = (uint8_t)(lba >> 8);
    *(volatile uint8_t *)(base + ATA_REG_LBAHI) = (uint8_t)(lba >> 16);
    
    // Send write command
    *(volatile uint8_t *)(base + ATA_REG_COMMAND) = ATA_CMD_WRITE_PIO;
    
    // Write sectors
    for (uint16_t i = 0; i < count; i++) {
        ata_wait_drq(channel);
        
        // Write 512 bytes (256 words)
        uint16_t *data_port = (uint16_t *)(base + ATA_REG_DATA);
        for (int j = 0; j < 256; j++) {
            uint16_t word = (buffer[i * 512 + j * 2]) | 
                           ((buffer[i * 512 + j * 2 + 1]) << 8);
            *data_port = word;
        }
    }
    
    // Flush cache
    ata_flush_cache(channel, drive);
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "sectors OK\n");
    #endif
    
    return 1;
}

/**
 * ata_flush_cache()
 * Flush write cache on device
 */
int ata_flush_cache(ata_channel_t channel, uint8_t drive)
{
    uint16_t base = (channel == ATA_CHANNEL_PRIMARY) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
    
    // Select drive
    *(volatile uint8_t *)(base + ATA_REG_DEVICE) = (drive == 0) ? 0xE0 : 0xF0;
    
    // Send flush cache command
    *(volatile uint8_t *)(base + ATA_REG_COMMAND) = ATA_CMD_FLUSH_CACHE;
    
    ata_wait_busy(channel);
    
    return 1;
}

/**
 * ata_detect_devices()
 * Detect all ATA devices
 */
int ata_detect_devices(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ATA] Detecting devices...\n");
    #endif
    
    int found = 0;
    
    // Check primary channel, master
    if (ata_identify_device(ATA_CHANNEL_PRIMARY, 0)) {
        found++;
    }
    
    // Check primary channel, slave
    if (ata_identify_device(ATA_CHANNEL_PRIMARY, 1)) {
        found++;
    }
    
    // Check secondary channel, master
    if (ata_identify_device(ATA_CHANNEL_SECONDARY, 0)) {
        found++;
    }
    
    // Check secondary channel, slave
    if (ata_identify_device(ATA_CHANNEL_SECONDARY, 1)) {
        found++;
    }
    
    ata_state.devices_found = found;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ATA] Found ");
    serial_putchar(SERIAL_PORT_A, '0' + found);
    serial_puts(SERIAL_PORT_A, " device(s)\n");
    #endif
    
    return found;
}

/**
 * ata_init()
 * Initialize ATA subsystem
 */
int ata_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ATA] Initializing ATA driver\n");
    #endif
    
    // Soft reset both channels
    ata_soft_reset(ATA_CHANNEL_PRIMARY);
    ata_soft_reset(ATA_CHANNEL_SECONDARY);
    
    // Detect devices
    ata_detect_devices();
    
    ata_state.initialized = 1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ATA] ATA driver initialized\n");
    #endif
    
    return 1;
}
