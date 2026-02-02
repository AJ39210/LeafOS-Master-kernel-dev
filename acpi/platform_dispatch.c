/**
 * drivers/acpi/platform_dispatch.c
 * 
 * ACPI Platform Dispatcher
 * 
 * Routes ACPI operations to platform-specific implementations
 * based on detected hardware
 */

#include "acpi.h"
#include "../../include/serial.h"

// Forward declarations for platform implementations
int qemu_acpi_init(void);
int vmbox_acpi_init(void);
int hp_acpi_init(void);

void qemu_acpi_shutdown(void);
void vmbox_acpi_shutdown(void);
void hp_acpi_shutdown(void);

int qemu_acpi_sleep(acpi_power_state_t state);
int vmbox_acpi_sleep(acpi_power_state_t state);
int hp_acpi_sleep(acpi_power_state_t state);

int qemu_acpi_apply_quirks(void);
int vmbox_acpi_apply_quirks(void);
int hp_acpi_apply_quirks(void);

void qemu_acpi_get_info(void);
void vmbox_acpi_get_info(void);
void hp_acpi_get_info(void);

/**
 * acpi_platform_init()
 * Initialize platform-specific ACPI support
 */
int acpi_platform_init(acpi_platform_t platform)
{
    switch (platform) {
        case ACPI_PLATFORM_QEMU:
            return qemu_acpi_init();
            
        case ACPI_PLATFORM_VIRTUALBOX:
            return vmbox_acpi_init();
            
        case ACPI_PLATFORM_HP:
            return hp_acpi_init();
            
        default:
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[ACPI] Unknown platform, using generic ACPI\n");
            #endif
            return 1;
    }
}

/**
 * acpi_platform_apply_quirks()
 * Apply platform-specific workarounds
 */
int acpi_platform_apply_quirks(void)
{
    switch (acpi_global_state.platform) {
        case ACPI_PLATFORM_QEMU:
            return qemu_acpi_apply_quirks();
            
        case ACPI_PLATFORM_VIRTUALBOX:
            return vmbox_acpi_apply_quirks();
            
        case ACPI_PLATFORM_HP:
            return hp_acpi_apply_quirks();
            
        default:
            return 1;
    }
}

/**
 * acpi_platform_sleep_prepare()
 * Prepare for sleep/power state change
 */
void acpi_platform_sleep_prepare(acpi_power_state_t state)
{
    switch (acpi_global_state.platform) {
        case ACPI_PLATFORM_QEMU:
            qemu_acpi_sleep(state);
            break;
            
        case ACPI_PLATFORM_VIRTUALBOX:
            vmbox_acpi_sleep(state);
            break;
            
        case ACPI_PLATFORM_HP:
            hp_acpi_sleep(state);
            break;
            
        default:
            break;
    }
}

/**
 * acpi_platform_shutdown()
 * Perform platform-specific shutdown
 */
void acpi_platform_shutdown(void)
{
    switch (acpi_global_state.platform) {
        case ACPI_PLATFORM_QEMU:
            qemu_acpi_shutdown();
            break;
            
        case ACPI_PLATFORM_VIRTUALBOX:
            vmbox_acpi_shutdown();
            break;
            
        case ACPI_PLATFORM_HP:
            hp_acpi_shutdown();
            break;
            
        default:
            break;
    }
}

/**
 * acpi_platform_get_info()
 * Print platform-specific information
 */
void acpi_platform_get_info(void)
{
    switch (acpi_global_state.platform) {
        case ACPI_PLATFORM_QEMU:
            qemu_acpi_get_info();
            break;
            
        case ACPI_PLATFORM_VIRTUALBOX:
            vmbox_acpi_get_info();
            break;
            
        case ACPI_PLATFORM_HP:
            hp_acpi_get_info();
            break;
            
        default:
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[ACPI] Generic platform (no specific optimizations)\n");
            #endif
            break;
    }
}
