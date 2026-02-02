/**
 * drivers/sata/sata.h
 * 
 * LeafOS SATA (Serial ATA) Driver
 * 
 * Provides support for SATA disk drives via AHCI (Advanced Host Controller Interface)
 * Supports hot-plug detection and native command queuing
 */

#ifndef SATA_H
#define SATA_H

#include <stdint.h>

/* AHCI HBA Memory Space */
#define AHCI_PORT_BASE          0x00000100
#define AHCI_PORT_SIZE          0x00000080

/* AHCI Global Registers */
#define AHCI_REG_CAP            0x00    /* Capabilities */
#define AHCI_REG_GHC            0x04    /* Global Host Control */
#define AHCI_REG_IS             0x08    /* Interrupt Status */
#define AHCI_REG_PI             0x0C    /* Ports Implemented */
#define AHCI_REG_VS             0x10    /* Version */

/* AHCI Port Registers (offset from AHCI_PORT_BASE) */
#define AHCI_PORT_REG_CLB       0x00    /* Command List Base Address */
#define AHCI_PORT_REG_CLBU      0x04    /* Command List Base Address Upper */
#define AHCI_PORT_REG_FB        0x08    /* FIS Base Address */
#define AHCI_PORT_REG_FBU       0x0C    /* FIS Base Address Upper */
#define AHCI_PORT_REG_IS        0x10    /* Interrupt Status */
#define AHCI_PORT_REG_IE        0x14    /* Interrupt Enable */
#define AHCI_PORT_REG_CMD       0x18    /* Command */
#define AHCI_PORT_REG_TFD       0x20    /* Task File Data */
#define AHCI_PORT_REG_SIG       0x24    /* Signature */
#define AHCI_PORT_REG_SSTS      0x28    /* Serial ATA Status */
#define AHCI_PORT_REG_SCTL      0x2C    /* Serial ATA Control */
#define AHCI_PORT_REG_SERR      0x30    /* Serial ATA Error */
#define AHCI_PORT_REG_SACT      0x34    /* Serial ATA Active */
#define AHCI_PORT_REG_CI        0x38    /* Command Issue */
#define AHCI_PORT_REG_SNTF      0x3C    /* Serial ATA Notification */

/* GHC Bits */
#define AHCI_GHC_AE             0x80000000  /* AHCI Enable */
#define AHCI_GHC_IE             0x00000002  /* Interrupt Enable */
#define AHCI_GHC_HR             0x00000001  /* Hard Reset */

/* Port SSTS Status */
#define AHCI_SSTS_DET_MASK      0x0000000F
#define AHCI_SSTS_DET_NO_LINK   0x00000000
#define AHCI_SSTS_DET_PRESENT   0x00000003

/* SATA Device Types */
typedef enum {
    SATA_DEVICE_NONE = 0,
    SATA_DEVICE_ATA = 1,       /* SATA drive */
    SATA_DEVICE_ATAPI = 2      /* SATA CDROM/DVD */
} sata_device_type_t;

/* SATA Device Info */
typedef struct {
    uint32_t port_num;
    sata_device_type_t type;
    uint32_t sectors;
    uint16_t cylinders;
    uint16_t heads;
    uint16_t spt;
    char model[41];
    char serial[21];
} sata_device_t;

/* AHCI Command Header Structure */
typedef struct {
    uint16_t opts;              /* Command FIS length, ATAPI, write indicator */
    uint16_t prdtl;             /* Physical Region Descriptor Table Length */
    uint32_t prdbc;             /* Physical Region Descriptor Byte Count */
    uint32_t ctba;              /* Command Table Base Address */
    uint32_t ctbau;             /* Command Table Base Address Upper */
    uint32_t reserved[4];
} __attribute__((packed)) ahci_cmd_header_t;

/* AHCI FIS Structure */
typedef struct {
    /* H2D FIS */
    uint8_t fis_type;           /* 0x27 for host-to-device */
    uint8_t pm_port;            /* Port multiplier port */
    uint8_t command;
    uint8_t features;
    uint32_t lba_low;           /* LBA[0:31] */
    uint32_t lba_high;          /* LBA[32:47] */
    uint32_t reserved;
    uint16_t count;             /* Sector count */
    uint8_t icc;                /* Isochronous command completion */
    uint8_t control;
    uint32_t reserved2;
} __attribute__((packed)) ahci_h2d_fis_t;

/* AHCI Physical Region Descriptor */
typedef struct {
    uint32_t dba;               /* Data Base Address */
    uint32_t dbau;              /* Data Base Address Upper */
    uint32_t reserved;
    uint32_t dbc;               /* Data Byte Count (0-based) */
} __attribute__((packed)) ahci_prd_t;

/* AHCI Command Table */
typedef struct {
    ahci_h2d_fis_t cfis;        /* Command FIS (64 bytes) */
    uint8_t atapi_cmd[16];      /* ATAPI command (if command type = 1) */
    uint8_t reserved[48];
    ahci_prd_t prd_table[248];  /* PRD entries (up to 248 for 4K alignment) */
} __attribute__((packed)) ahci_cmd_table_t;

/* AHCI Received FIS Structure */
typedef struct {
    uint8_t dma_setup_fis[28];
    uint8_t reserved1[4];
    uint8_t pio_setup_fis[20];
    uint8_t reserved2[12];
    uint8_t d2h_register_fis[20];
    uint8_t reserved3[4];
    uint8_t set_device_bits_fis[8];
    uint8_t unknown_fis[64];
    uint8_t reserved4[96];
} __attribute__((packed)) ahci_received_fis_t;

/* Global SATA State */
typedef struct {
    uint32_t ahci_base;
    int initialized;
    int devices_found;
    sata_device_t devices[32];  /* Max 32 ports */
    
    /* Command lists per port (if implemented) */
    void *cmd_list[32];
    void *cmd_table[32];
    void *rx_fis[32];
} sata_state_t;

/* SATA Commands */
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_IDENTIFY_DEVICE 0xEC

/* Core Functions */
int sata_init(void);
int sata_probe_ports(void);
int sata_detect_device(uint32_t port);

/* I/O Functions */
int sata_read_sectors(uint32_t port, uint32_t lba, uint16_t count, uint8_t *buffer);
int sata_write_sectors(uint32_t port, uint32_t lba, uint16_t count, uint8_t *buffer);

/* Control Functions */
int sata_port_reset(uint32_t port);
int sata_port_enable(uint32_t port);
int sata_port_disable(uint32_t port);

/* Utility Functions */
uint32_t sata_read_port_status(uint32_t port);
int sata_wait_device_ready(uint32_t port);
void sata_print_device_info(uint32_t port);

#endif // SATA_H
