/**
 * drivers/acpi/acpi.c
 * 
 * LeafOS ACPI Driver - Main Implementation
 * 
 * Provides multi-platform ACPI support with automatic detection
 * and platform-specific workarounds for buggy BIOS implementations
 */

#include "acpi.h"
#include "../../include/serial.h"
#include <stddef.h>

// Global ACPI state
acpi_state_t acpi_global_state = {0};

/**
 * acpi_checksum()
 * Verify ACPI table checksum
 */
uint8_t acpi_checksum(uint8_t *table, uint32_t length)
{
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; i++) {
        sum += table[i];
    }
    return sum;
}

/**
 * acpi_get_platform_name()
 * Return human-readable platform name
 */
char* acpi_get_platform_name(acpi_platform_t platform)
{
    switch (platform) {
        case ACPI_PLATFORM_QEMU:
            return "QEMU";
        case ACPI_PLATFORM_VMWARE:
            return "VMware";
        case ACPI_PLATFORM_VIRTUALBOX:
            return "VirtualBox";
        case ACPI_PLATFORM_HP:
            return "HP (Real Hardware)";
        case ACPI_PLATFORM_DELL:
            return "Dell";
        case ACPI_PLATFORM_LENOVO:
            return "Lenovo";
        default:
            return "Unknown";
    }
}

/**
 * acpi_find_rsdp()
 * Search for RSDP in EBDA and BIOS ROM areas
 * Returns 1 if found, 0 otherwise
 */
int acpi_find_rsdp(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI] Searching for RSDP...\n");
    #endif
    
    // Search EBDA (Extended BIOS Data Area) at 0x40E
    uint16_t ebda_addr = *(uint16_t *)0x40E * 16;
    
    // Search from EBDA start through EBDA + 1KB
    uint8_t *ptr = (uint8_t *)ebda_addr;
    for (int i = 0; i < 1024; i += 16) {
        if (*(uint32_t *)(ptr + i) == 0x20445352) {  // "RSD " signature
            acpi_global_state.rsdp = (acpi_rsdp_t *)(ptr + i);
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[ACPI] RSDP found at EBDA\n");
            #endif
            return 1;
        }
    }
    
    // Search BIOS ROM area (0xE0000 - 0xFFFFF)
    ptr = (uint8_t *)0xE0000;
    while ((uint32_t)ptr < 0x100000) {
        if (*(uint32_t *)ptr == 0x20445352) {  // "RSD " signature
            acpi_global_state.rsdp = (acpi_rsdp_t *)ptr;
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[ACPI] RSDP found in BIOS ROM\n");
            #endif
            return 1;
        }
        ptr += 16;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI] RSDP not found (ACPI not available)\n");
    #endif
    return 0;
}

/**
 * acpi_detect_platform()
 * Auto-detect the hardware platform based on BIOS information
 */
int acpi_detect_platform(void)
{
    // Try to detect platform from DMI data or BIOS signatures
    // For now, we'll check common signatures
    
    uint8_t *bios_area = (uint8_t *)0xF0000;
    
    // Search for platform signatures in BIOS
    for (uint32_t i = 0; i < 65536; i++) {
        // Check for QEMU signature
        if (i + 4 < 65536) {
            if (*(uint32_t *)(bios_area + i) == 0x4D455151) {  // "QEMU"
                acpi_global_state.platform = ACPI_PLATFORM_QEMU;
                return 1;
            }
        }
        
        // Check for VirtualBox signature
        if (i + 11 < 65536) {
            uint8_t *str = bios_area + i;
            if (str[0] == 'V' && str[1] == 'i' && str[2] == 'r' && 
                str[3] == 't' && str[4] == 'u' && str[5] == 'a' && 
                str[6] == 'l' && str[7] == 'B' && str[8] == 'o' && 
                str[9] == 'x') {
                acpi_global_state.platform = ACPI_PLATFORM_VIRTUALBOX;
                return 1;
            }
        }
    }
    
    // Default to unknown (will use generic ACPI)
    acpi_global_state.platform = ACPI_PLATFORM_UNKNOWN;
    return 0;
}

/**
 * acpi_parse_tables()
 * Parse ACPI tables and extract key information
 */
int acpi_parse_tables(void)
{
    if (!acpi_global_state.rsdp) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI] Parsing tables...\n");
    #endif
    
    // For now, minimal parsing
    // Full implementation would traverse RSDT/XSDT and parse FADT, MADT, etc.
    
    return 1;
}

/**
 * acpi_enable()
 * Enable ACPI mode on the system
 */
int acpi_enable(void)
{
    if (!acpi_global_state.rsdp || !acpi_global_state.fadt) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI] Enabling ACPI mode\n");
    #endif
    
    // Send ACPI enable command via SMI (System Management Interrupt)
    // This is platform-specific and handled by platform drivers
    
    acpi_global_state.initialized = 1;
    return 1;
}

/**
 * acpi_disable()
 * Disable ACPI mode
 */
int acpi_disable(void)
{
    acpi_global_state.initialized = 0;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI] ACPI disabled\n");
    #endif
    
    return 1;
}

/**
 * acpi_set_power_state()
 * Request a power state change (sleep, shutdown, etc.)
 */
int acpi_set_power_state(acpi_power_state_t state)
{
    if (!acpi_global_state.initialized) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    char msg[64];
    const char *state_names[] = {"S0", "S1", "S3", "S4", "S5"};
    if (state <= ACPI_POWER_S5) {
        serial_puts(SERIAL_PORT_A, "[ACPI] Requesting power state: ");
        serial_puts(SERIAL_PORT_A, (char*)state_names[state]);
        serial_puts(SERIAL_PORT_A, "\n");
    }
    #endif
    
    // Platform-specific implementation
    acpi_platform_sleep_prepare(state);
    
    return 1;
}

/**
 * acpi_get_power_state()
 * Get current power state
 */
int acpi_get_power_state(void)
{
    return ACPI_POWER_S0;  // Always running for now
}

/**
 * acpi_register_event_handler()
 * Register handler for ACPI events
 */
int acpi_register_event_handler(acpi_event_type_t type)
{
    acpi_global_state.event_handlers |= (1 << type);
    return 1;
}

/**
 * acpi_unregister_event_handler()
 * Unregister event handler
 */
int acpi_unregister_event_handler(acpi_event_type_t type)
{
    acpi_global_state.event_handlers &= ~(1 << type);
    return 1;
}

/**
 * acpi_handle_event()
 * Handle ACPI events
 */
void acpi_handle_event(acpi_event_type_t type)
{
    if (!(acpi_global_state.event_handlers & (1 << type))) {
        return;
    }
    
    switch (type) {
        case ACPI_EVENT_POWER_BUTTON:
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[ACPI] Power button pressed\n");
            #endif
            break;
        case ACPI_EVENT_SLEEP_BUTTON:
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[ACPI] Sleep button pressed\n");
            #endif
            acpi_set_power_state(ACPI_POWER_S1);
            break;
        case ACPI_EVENT_THERMAL:
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[ACPI] Thermal event\n");
            #endif
            break;
        default:
            break;
    }
}

/**
 * acpi_print_fadt_info()
 * Print FADT information for debugging
 */
void acpi_print_fadt_info(void)
{
    if (!acpi_global_state.fadt) {
        return;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI] FADT Information:\n");
    serial_puts(SERIAL_PORT_A, "[ACPI]   SCI Interrupt: ");
    #endif
}

/**
 * acpi_init()
 * Initialize ACPI subsystem
 */
int acpi_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI] Initializing ACPI subsystem\n");
    #endif
    
    // Step 1: Detect platform
    acpi_detect_platform();
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI] Platform: ");
    serial_puts(SERIAL_PORT_A, acpi_get_platform_name(acpi_global_state.platform));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    // Step 2: Find RSDP
    if (!acpi_find_rsdp()) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[ACPI] ACPI not available on this system\n");
        #endif
        return 0;
    }
    
    // Step 3: Platform-specific initialization
    if (!acpi_platform_init(acpi_global_state.platform)) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[ACPI] Platform initialization failed\n");
        #endif
        return 0;
    }
    
    // Step 4: Apply platform-specific quirks
    acpi_platform_apply_quirks();
    
    // Step 5: Parse ACPI tables
    acpi_parse_tables();
    
    // Step 6: Enable ACPI
    acpi_enable();
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI] ACPI subsystem initialized successfully\n");
    #endif
    
    return 1;
}
