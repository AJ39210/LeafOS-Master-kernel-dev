/**
 * drivers/sata/sata.c
 * 
 * LeafOS SATA Driver Implementation
 * 
 * AHCI (Advanced Host Controller Interface) compliant
 * Supports hot-plug detection and port enumeration
 */

#include "sata.h"
#include "../../include/serial.h"
#include <stddef.h>

static sata_state_t sata_state = {0};

/**
 * sata_read_port_status()
 * Read port status register
 */
uint32_t sata_read_port_status(uint32_t port)
{
    if (!sata_state.ahci_base || port >= 32) {
        return 0;
    }
    
    uint32_t port_base = sata_state.ahci_base + AHCI_PORT_BASE + (port * AHCI_PORT_SIZE);
    uint32_t ssts = *(volatile uint32_t *)(port_base + AHCI_PORT_REG_SSTS);
    
    return ssts;
}

/**
 * sata_port_reset()
 * Reset SATA port
 */
int sata_port_reset(uint32_t port)
{
    if (!sata_state.ahci_base || port >= 32) {
        return 0;
    }
    
    uint32_t port_base = sata_state.ahci_base + AHCI_PORT_BASE + (port * AHCI_PORT_SIZE);
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] Resetting port ");
    serial_putchar(SERIAL_PORT_A, '0' + (port % 10));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    // Set port SCTL for hard reset (DET=1)
    *(volatile uint32_t *)(port_base + AHCI_PORT_REG_SCTL) = 0x00000001;
    
    // Wait
    for (volatile int i = 0; i < 10000; i++);
    
    // Clear DET
    *(volatile uint32_t *)(port_base + AHCI_PORT_REG_SCTL) = 0x00000000;
    
    // Wait for link to establish
    int timeout = 10000;
    while (timeout-- > 0) {
        uint32_t ssts = *(volatile uint32_t *)(port_base + AHCI_PORT_REG_SSTS);
        if ((ssts & AHCI_SSTS_DET_MASK) == AHCI_SSTS_DET_PRESENT) {
            break;
        }
    }
    
    return 1;
}

/**
 * sata_port_enable()
 * Enable SATA port
 */
int sata_port_enable(uint32_t port)
{
    if (!sata_state.ahci_base || port >= 32) {
        return 0;
    }
    
    uint32_t port_base = sata_state.ahci_base + AHCI_PORT_BASE + (port * AHCI_PORT_SIZE);
    
    // Start command engine: set ST (Start) bit
    volatile uint32_t *cmd_reg = (volatile uint32_t *)(port_base + AHCI_PORT_REG_CMD);
    *cmd_reg |= 0x00000001;  /* Start */
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] Port ");
    serial_putchar(SERIAL_PORT_A, '0' + (port % 10));
    serial_puts(SERIAL_PORT_A, " enabled\n");
    #endif
    
    return 1;
}

/**
 * sata_port_disable()
 * Disable SATA port
 */
int sata_port_disable(uint32_t port)
{
    if (!sata_state.ahci_base || port >= 32) {
        return 0;
    }
    
    uint32_t port_base = sata_state.ahci_base + AHCI_PORT_BASE + (port * AHCI_PORT_SIZE);
    
    // Clear ST bit
    volatile uint32_t *cmd_reg = (volatile uint32_t *)(port_base + AHCI_PORT_REG_CMD);
    *cmd_reg &= ~0x00000001;  /* Stop */
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] Port ");
    serial_putchar(SERIAL_PORT_A, '0' + (port % 10));
    serial_puts(SERIAL_PORT_A, " disabled\n");
    #endif
    
    return 1;
}

/**
 * sata_wait_device_ready()
 * Wait for device to be ready
 */
int sata_wait_device_ready(uint32_t port)
{
    if (!sata_state.ahci_base || port >= 32) {
        return 0;
    }
    
    uint32_t port_base = sata_state.ahci_base + AHCI_PORT_BASE + (port * AHCI_PORT_SIZE);
    
    int timeout = 10000;
    while (timeout-- > 0) {
        uint32_t ssts = *(volatile uint32_t *)(port_base + AHCI_PORT_REG_SSTS);
        if ((ssts & AHCI_SSTS_DET_MASK) == AHCI_SSTS_DET_PRESENT) {
            return 1;
        }
    }
    
    return 0;
}

/**
 * sata_detect_device()
 * Detect device on SATA port
 */
int sata_detect_device(uint32_t port)
{
    if (!sata_state.ahci_base || port >= 32) {
        return 0;
    }
    
    uint32_t port_base = sata_state.ahci_base + AHCI_PORT_BASE + (port * AHCI_PORT_SIZE);
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] Detecting device on port ");
    serial_putchar(SERIAL_PORT_A, '0' + (port % 10));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    // Check port status
    uint32_t ssts = *(volatile uint32_t *)(port_base + AHCI_PORT_REG_SSTS);
    
    if ((ssts & AHCI_SSTS_DET_MASK) != AHCI_SSTS_DET_PRESENT) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[SATA] No device on port ");
        serial_putchar(SERIAL_PORT_A, '0' + (port % 10));
        serial_puts(SERIAL_PORT_A, "\n");
        #endif
        return 0;
    }
    
    // Check device signature
    uint32_t sig = *(volatile uint32_t *)(port_base + AHCI_PORT_REG_SIG);
    
    sata_device_type_t type = SATA_DEVICE_NONE;
    if (sig == 0x00000101) {
        type = SATA_DEVICE_ATA;
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[SATA] SATA ATA drive detected\n");
        #endif
    } else if (sig == 0xEB140101) {
        type = SATA_DEVICE_ATAPI;
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[SATA] SATA ATAPI device detected\n");
        #endif
    }
    
    if (type != SATA_DEVICE_NONE) {
        sata_state.devices[port].port_num = port;
        sata_state.devices[port].type = type;
        sata_state.devices_found++;
        return 1;
    }
    
    return 0;
}

/**
 * sata_setup_port()
 * Setup command list and buffers for a port
 */
static int sata_setup_port(uint32_t port)
{
    /* Allocate command list (1024 bytes per port) */
    uint8_t *cmd_list_mem = (uint8_t *)0x10000 + (port * 0x2000);  /* Arbitrary memory location */
    sata_state.cmd_list[port] = cmd_list_mem;
    
    /* Allocate command table (also for slot 0) */
    uint8_t *cmd_table_mem = cmd_list_mem + 0x1000;
    sata_state.cmd_table[port] = cmd_table_mem;
    
    /* Allocate received FIS */
    uint8_t *rx_fis_mem = cmd_list_mem + 0x1400;
    sata_state.rx_fis[port] = rx_fis_mem;
    
    uint32_t port_base = sata_state.ahci_base + AHCI_PORT_BASE + (port * AHCI_PORT_SIZE);
    
    /* Set command list base address */
    *(volatile uint32_t *)(port_base + AHCI_PORT_REG_CLB) = (uint32_t)cmd_list_mem;
    *(volatile uint32_t *)(port_base + AHCI_PORT_REG_CLBU) = 0;
    
    /* Set received FIS base address */
    *(volatile uint32_t *)(port_base + AHCI_PORT_REG_FB) = (uint32_t)rx_fis_mem;
    *(volatile uint32_t *)(port_base + AHCI_PORT_REG_FBU) = 0;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] Port ");
    serial_putchar(SERIAL_PORT_A, '0' + (port % 10));
    serial_puts(SERIAL_PORT_A, " buffers initialized\n");
    #endif
    
    return 1;
}

/**
 * sata_probe_ports()
 * Probe all SATA ports
 */
int sata_probe_ports(void)
{
    if (!sata_state.ahci_base) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] Probing ports...\n");
    #endif
    
    // Read ports implemented register
    uint32_t pi = *(volatile uint32_t *)(sata_state.ahci_base + AHCI_REG_PI);
    
    int found = 0;
    for (uint32_t port = 0; port < 32; port++) {
        if (pi & (1 << port)) {
            /* Setup port buffers */
            sata_setup_port(port);
            
            /* Reset and enable port */
            sata_port_reset(port);
            
            if (sata_wait_device_ready(port)) {
                sata_port_enable(port);
                if (sata_detect_device(port)) {
                    found++;
                }
            }
        }
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] Found ");
    serial_putchar(SERIAL_PORT_A, '0' + found);
    serial_puts(SERIAL_PORT_A, " device(s)\n");
    #endif
    
    return found;
}

/**
 * sata_build_read_command()
 * Build READ DMA EXT command FIS
 */
static void sata_build_read_command(ahci_h2d_fis_t *fis, uint32_t lba, uint16_t count)
{
    fis->fis_type = 0x27;           /* Host-to-device FIS */
    fis->pm_port = 0x80;            /* C bit set, port 0 */
    fis->command = ATA_CMD_READ_DMA_EXT;
    fis->features = 0;
    fis->lba_low = lba & 0xFFFFFFFF;
    fis->lba_high = (lba >> 32) & 0xFFFF;
    fis->count = count;
    fis->icc = 0;
    fis->control = 0;
}

/**
 * sata_build_write_command()
 * Build WRITE DMA EXT command FIS
 */
static void sata_build_write_command(ahci_h2d_fis_t *fis, uint32_t lba, uint16_t count)
{
    fis->fis_type = 0x27;           /* Host-to-device FIS */
    fis->pm_port = 0x80;            /* C bit set, port 0 */
    fis->command = ATA_CMD_WRITE_DMA_EXT;
    fis->features = 0;
    fis->lba_low = lba & 0xFFFFFFFF;
    fis->lba_high = (lba >> 32) & 0xFFFF;
    fis->count = count;
    fis->icc = 0;
    fis->control = 0;
}

/**
 * sata_submit_command()
 * Submit command to port and wait for completion
 */
static int sata_submit_command(uint32_t port, int slot)
{
    if (!sata_state.ahci_base || port >= 32) {
        return 0;
    }
    
    uint32_t port_base = sata_state.ahci_base + AHCI_PORT_BASE + (port * AHCI_PORT_SIZE);
    
    /* Issue command by writing to CI register */
    volatile uint32_t *ci_reg = (volatile uint32_t *)(port_base + AHCI_PORT_REG_CI);
    *ci_reg = (1 << slot);
    
    /* Wait for completion with timeout */
    int timeout = 100000;
    while (timeout-- > 0) {
        if (!(*ci_reg & (1 << slot))) {
            /* Command completed */
            return 1;
        }
        
        /* Check for errors */
        volatile uint32_t *is_reg = (volatile uint32_t *)(port_base + AHCI_PORT_REG_IS);
        if (*is_reg & 0x40000000) {  /* TFES - Task File Error Status */
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[SATA] Task file error on port ");
            serial_putchar(SERIAL_PORT_A, '0' + (port % 10));
            serial_puts(SERIAL_PORT_A, "\n");
            #endif
            return 0;
        }
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] Command timeout on port ");
    serial_putchar(SERIAL_PORT_A, '0' + (port % 10));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return 0;
}

/**
 * sata_read_sectors()
 * Read sectors from SATA device using DMA
 */
int sata_read_sectors(uint32_t port, uint32_t lba, uint16_t count, uint8_t *buffer)
{
    if (!buffer || !sata_state.initialized || port >= 32) {
        return 0;
    }
    
    if (sata_state.devices[port].type == SATA_DEVICE_NONE) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] Read ");
    serial_putchar(SERIAL_PORT_A, '0' + ((count >> 8) & 0xF));
    serial_putchar(SERIAL_PORT_A, '0' + (count & 0xF));
    serial_puts(SERIAL_PORT_A, " sectors from port ");
    serial_putchar(SERIAL_PORT_A, '0' + (port % 10));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    uint32_t port_base = sata_state.ahci_base + AHCI_PORT_BASE + (port * AHCI_PORT_SIZE);
    
    /* Get command table from port (slot 0) */
    if (!sata_state.cmd_table[port]) {
        return 0;
    }
    
    ahci_cmd_table_t *cmd_table = (ahci_cmd_table_t *)sata_state.cmd_table[port];
    
    /* Build command FIS */
    sata_build_read_command(&cmd_table->cfis, lba, count);
    
    /* Setup Physical Region Descriptor for data buffer */
    cmd_table->prd_table[0].dba = (uint32_t)buffer;
    cmd_table->prd_table[0].dbau = 0;
    cmd_table->prd_table[0].dbc = (count * 512) - 1;  /* Byte count (0-based) */
    cmd_table->prd_table[0].reserved = 0x80000000;    /* I bit (last PRD) */
    
    /* Update command header */
    ahci_cmd_header_t *cmd_header = (ahci_cmd_header_t *)sata_state.cmd_list[port];
    cmd_header->opts = 5;           /* FIS length in DWORDs (5 for 20 bytes) */
    cmd_header->prdtl = 1;          /* 1 PRD entry */
    cmd_header->ctba = (uint32_t)cmd_table;
    cmd_header->ctbau = 0;
    
    /* Submit command (slot 0) */
    return sata_submit_command(port, 0);
}

/**
 * sata_write_sectors()
 * Write sectors to SATA device using DMA
 */
int sata_write_sectors(uint32_t port, uint32_t lba, uint16_t count, uint8_t *buffer)
{
    if (!buffer || !sata_state.initialized || port >= 32) {
        return 0;
    }
    
    if (sata_state.devices[port].type == SATA_DEVICE_NONE) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] Write ");
    serial_putchar(SERIAL_PORT_A, '0' + ((count >> 8) & 0xF));
    serial_putchar(SERIAL_PORT_A, '0' + (count & 0xF));
    serial_puts(SERIAL_PORT_A, " sectors to port ");
    serial_putchar(SERIAL_PORT_A, '0' + (port % 10));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    uint32_t port_base = sata_state.ahci_base + AHCI_PORT_BASE + (port * AHCI_PORT_SIZE);
    
    /* Get command table from port (slot 0) */
    if (!sata_state.cmd_table[port]) {
        return 0;
    }
    
    ahci_cmd_table_t *cmd_table = (ahci_cmd_table_t *)sata_state.cmd_table[port];
    
    /* Build command FIS */
    sata_build_write_command(&cmd_table->cfis, lba, count);
    
    /* Setup Physical Region Descriptor for data buffer */
    cmd_table->prd_table[0].dba = (uint32_t)buffer;
    cmd_table->prd_table[0].dbau = 0;
    cmd_table->prd_table[0].dbc = (count * 512) - 1;  /* Byte count (0-based) */
    cmd_table->prd_table[0].reserved = 0x80000000;    /* I bit (last PRD) + write flag */
    
    /* Update command header */
    ahci_cmd_header_t *cmd_header = (ahci_cmd_header_t *)sata_state.cmd_list[port];
    cmd_header->opts = 5 | 0x40000000;  /* FIS length + write bit (W) */
    cmd_header->prdtl = 1;              /* 1 PRD entry */
    cmd_header->ctba = (uint32_t)cmd_table;
    cmd_header->ctbau = 0;
    
    /* Submit command (slot 0) */
    return sata_submit_command(port, 0);
}

/**
 * sata_init()
 * Initialize SATA subsystem
 */
int sata_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] Initializing SATA driver\n");
    #endif
    
    // Normally would scan PCI for AHCI controller
    // For now, use default AHCI base (would be found via PCI)
    sata_state.ahci_base = 0;  // Would be set by PCI scan
    
    if (!sata_state.ahci_base) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[SATA] No AHCI controller found\n");
        #endif
        return 0;
    }
    
    // Enable AHCI
    *(volatile uint32_t *)(sata_state.ahci_base + AHCI_REG_GHC) |= AHCI_GHC_AE;
    
    // Probe ports
    sata_probe_ports();
    
    sata_state.initialized = 1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SATA] SATA driver initialized\n");
    #endif
    
    return 1;
}
