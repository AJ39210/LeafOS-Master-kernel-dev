/**
 * drivers/acpi/acpi.h
 * 
 * LeafOS ACPI (Advanced Configuration and Power Interface) Driver
 * 
 * Provides power management, thermal management, and device configuration
 * across multiple hardware platforms (QEMU, VirtualBox, HP real hardware)
 */

#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>

/* ACPI Table Signatures (found in RSDP) */
#define ACPI_SIGNATURE_RSDP 0x20445352  // "RSD " (in little-endian)
#define ACPI_SIGNATURE_RSDT 0x54445352  // "RSDT"
#define ACPI_SIGNATURE_XSDT 0x54445358  // "XSDT"
#define ACPI_SIGNATURE_FADT 0x54414446  // "FADT"
#define ACPI_SIGNATURE_MADT 0x54414D41  // "MADT"
#define ACPI_SIGNATURE_DSDT 0x54445344  // "DSDT"

/* ACPI Hardware Platform Types */
typedef enum {
    ACPI_PLATFORM_UNKNOWN = 0,
    ACPI_PLATFORM_QEMU = 1,
    ACPI_PLATFORM_VMWARE = 2,
    ACPI_PLATFORM_VIRTUALBOX = 3,
    ACPI_PLATFORM_HP = 4,
    ACPI_PLATFORM_DELL = 5,
    ACPI_PLATFORM_LENOVO = 6
} acpi_platform_t;

/* ACPI Power States */
typedef enum {
    ACPI_POWER_S0 = 0,  // Working (system on)
    ACPI_POWER_S1 = 1,  // Sleep (CPU powered)
    ACPI_POWER_S3 = 3,  // Sleep (memory powered)
    ACPI_POWER_S4 = 4,  // Hibernation (disk)
    ACPI_POWER_S5 = 5   // Shutdown (off)
} acpi_power_state_t;

/* ACPI Event Types */
typedef enum {
    ACPI_EVENT_POWER_BUTTON = 0,
    ACPI_EVENT_SLEEP_BUTTON = 1,
    ACPI_EVENT_THERMAL = 2,
    ACPI_EVENT_DEVICE_NOTIFY = 3,
    ACPI_EVENT_BATTERY = 4,
    ACPI_EVENT_LID = 5
} acpi_event_type_t;

/* ACPI RSDP Structure (Root System Description Pointer) */
typedef struct {
    uint8_t signature[8];     // "RSD PTR "
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;          // ACPI 2.0+
    uint64_t xsdt_address;    // ACPI 2.0+
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) acpi_rsdp_t;

/* ACPI Table Header */
typedef struct {
    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_table_header_t;

/* FADT (Fixed ACPI Description Table) */
typedef struct {
    acpi_table_header_t header;
    uint32_t firmware_ctrl;
    uint32_t dsdt_address;
    uint8_t reserved1;
    uint8_t preferred_pm_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t acpi_enable_command;
    uint8_t acpi_disable_command;
    uint8_t s4bios_request;
    uint8_t pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
} __attribute__((packed)) acpi_fadt_t;

/* ACPI Global State */
typedef struct {
    acpi_platform_t platform;
    int initialized;
    acpi_rsdp_t *rsdp;
    acpi_fadt_t *fadt;
    uint32_t event_handlers;
    uint8_t quirks_enabled;  // For buggy BIOS workarounds
} acpi_state_t;

// Global ACPI state (defined in acpi.c)
extern acpi_state_t acpi_global_state;

/* Core ACPI Functions */
int acpi_init(void);
int acpi_detect_platform(void);
int acpi_find_rsdp(void);
int acpi_parse_tables(void);
int acpi_enable(void);
int acpi_disable(void);
int acpi_set_power_state(acpi_power_state_t state);
int acpi_get_power_state(void);

/* Event Handling */
int acpi_register_event_handler(acpi_event_type_t type);
int acpi_unregister_event_handler(acpi_event_type_t type);
void acpi_handle_event(acpi_event_type_t type);

/* Platform-Specific Functions (implemented per platform) */
int acpi_platform_init(acpi_platform_t platform);
int acpi_platform_apply_quirks(void);
void acpi_platform_sleep_prepare(acpi_power_state_t state);
void acpi_platform_shutdown(void);

/* Utility Functions */
uint8_t acpi_checksum(uint8_t *table, uint32_t length);
char* acpi_get_platform_name(acpi_platform_t platform);
void acpi_print_fadt_info(void);

#endif // ACPI_H
