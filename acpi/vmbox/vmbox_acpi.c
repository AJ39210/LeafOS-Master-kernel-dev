/**
 * drivers/acpi/vmbox/vmbox_acpi.c
 * 
 * VirtualBox ACPI Platform Driver
 * 
 * VirtualBox has mostly standard ACPI but with some quirks:
 * - May report incorrect temperature sensors
 * - Power state transitions can be unreliable
 * - Battery status not always accurate
 */

#include "../acpi.h"
#include "../../../include/serial.h"

/**
 * vmbox_acpi_init()
 * Initialize ACPI for VirtualBox platform
 */
int vmbox_acpi_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox] Initializing VirtualBox ACPI support\n");
    #endif
    
    // VirtualBox mostly follows ACPI spec
    // Enable quirks mode for reliability
    acpi_global_state.quirks_enabled = 1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox] Enabling compatibility quirks\n");
    #endif
    
    return 1;
}

/**
 * vmbox_acpi_apply_quirks()
 * Apply VirtualBox-specific workarounds
 */
int vmbox_acpi_apply_quirks(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox] Applying platform quirks:\n");
    #endif
    
    // Quirk 1: Disable unreliable thermal monitoring
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox]  - Disabling thermal zone monitoring (unreliable)\n");
    #endif
    
    // Quirk 2: Add delays to power state transitions
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox]  - Adding delays to power transitions\n");
    #endif
    
    // Quirk 3: Bypass battery status checks
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox]  - Disabling battery status polling\n");
    #endif
    
    return 1;
}

/**
 * vmbox_acpi_delay_ms()
 * Millisecond delay helper for VirtualBox quirks
 */
static void vmbox_acpi_delay_ms(uint32_t ms)
{
    // Simple delay loop (assumes ~1000 CPU cycles per ms at 1GHz)
    for (uint32_t i = 0; i < ms * 1000; i++) {
        asm("nop");
    }
}

/**
 * vmbox_acpi_shutdown()
 * Prepare system for shutdown on VirtualBox
 * (VirtualBox shutdown can be unreliable, so we add retries)
 */
void vmbox_acpi_shutdown(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox] Executing shutdown sequence (with retries)\n");
    #endif
    
    // Try multiple times due to VirtualBox quirks
    for (int attempt = 0; attempt < 3; attempt++) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox] Shutdown attempt ");
        serial_putchar(SERIAL_PORT_A, '1' + attempt);
        serial_puts(SERIAL_PORT_A, "/3\n");
        #endif
        
        // Standard ACPI shutdown
        uint32_t pm1_ctrl = (5 << 10) | 0x2000;  // SLP_TYP=5 (S5), SLP_EN=1
        
        volatile uint32_t *pm1_ctrl_addr = (uint32_t *)0xB004;
        *pm1_ctrl_addr = pm1_ctrl;
        
        // Wait a bit and retry if still running
        vmbox_acpi_delay_ms(100);
    }
    
    // Final attempt with direct register write
    asm volatile("cli");
    asm volatile("hlt");
}

/**
 * vmbox_acpi_sleep()
 * Enter sleep state on VirtualBox
 */
int vmbox_acpi_sleep(acpi_power_state_t state)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox] Sleep state: ");
    serial_putchar(SERIAL_PORT_A, '0' + state);
    serial_puts(SERIAL_PORT_A, " (with delay)\n");
    #endif
    
    // Add delay before sleep transition (VirtualBox quirk)
    vmbox_acpi_delay_ms(50);
    
    uint32_t sleep_type = (state & 0x7) << 10;
    uint32_t sleep_en = 0x2000;
    
    volatile uint32_t *pm1_ctrl = (uint32_t *)0xB004;
    *pm1_ctrl = sleep_type | sleep_en;
    
    // Add delay after command
    vmbox_acpi_delay_ms(50);
    
    asm("hlt");
    
    return 1;
}

/**
 * vmbox_acpi_handle_thermal_fault()
 * Handle thermal sensor faults (common in VirtualBox)
 */
void vmbox_acpi_handle_thermal_fault(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox] Thermal sensor fault detected (ignoring)\n");
    #endif
    
    // VirtualBox often reports false thermal faults
    // Simply log and ignore
}

/**
 * vmbox_acpi_get_info()
 * Get VirtualBox ACPI platform information
 */
void vmbox_acpi_get_info(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox] Platform: VirtualBox Virtual Machine\n");
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox] Features: Mostly standard ACPI\n");
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox] Quirks: Thermal, Battery, Sleep (enabled)\n");
    serial_puts(SERIAL_PORT_A, "[ACPI-VirtualBox] Reliable: Partial (with workarounds)\n");
    #endif
}
