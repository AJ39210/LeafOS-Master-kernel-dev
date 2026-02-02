/**
 * kernel/SELeaf/driver_hooks.c
 * 
 * SELeaf Per-Driver Security Hooks
 * 
 * Provides:
 * - Security hooks for each driver
 * - DMA protection
 * - Interrupt handler validation
 * - Driver capability enforcement
 * - Driver-specific policies
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Driver security context */
typedef struct {
    uint32_t driver_id;
    char name[32];
    uint32_t capabilities;      /* Bitmask of allowed operations */
    uint32_t max_dma_size;      /* Max DMA transfer size */
    int dma_protection_enabled;
    int interrupt_validation;
    uint32_t allowed_irqs;      /* Bitmask of allowed IRQ lines */
} driver_security_t;

/* Driver hook types */
typedef enum {
    HOOK_DRIVER_INIT = 0,
    HOOK_DRIVER_LOAD,
    HOOK_DRIVER_UNLOAD,
    HOOK_DMA_REQUEST,
    HOOK_DMA_COMPLETE,
    HOOK_INTERRUPT_HANDLER,
    HOOK_IOCTL,
    HOOK_READ,
    HOOK_WRITE
} hook_type_t;

/* Security hook */
typedef struct {
    uint32_t driver_id;
    hook_type_t hook_type;
    int (*hook_func)(uint32_t driver_id, uint32_t arg1, uint32_t arg2);
} security_hook_t;

/* Driver hooks state */
#define DRIVER_MAX              32
#define HOOKS_PER_DRIVER        16

typedef struct {
    driver_security_t drivers[DRIVER_MAX];
    uint32_t driver_count;
    
    security_hook_t hooks[DRIVER_MAX * HOOKS_PER_DRIVER];
    uint32_t hook_count;
    
    uint32_t hook_executions;
    uint32_t hook_blocks;
} driver_hooks_state_t;

static driver_hooks_state_t hooks_state = {0};

/**
 * driver_hooks_init()
 * Initialize driver security hooks
 */
int driver_hooks_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[DRIVER_HOOKS] Driver security initialized\n");
    #endif
    
    hooks_state.driver_count = 0;
    hooks_state.hook_count = 0;
    hooks_state.hook_executions = 0;
    hooks_state.hook_blocks = 0;
    
    return 0;
}

/**
 * driver_register_security()
 * Register driver with security context
 */
uint32_t driver_register_security(const char *driver_name, uint32_t capabilities,
                                 uint32_t max_dma_size)
{
    if (hooks_state.driver_count >= DRIVER_MAX) {
        return 0;  /* Error */
    }
    
    driver_security_t *drv = &hooks_state.drivers[hooks_state.driver_count];
    drv->driver_id = hooks_state.driver_count + 1;
    
    /* Copy driver name */
    int i = 0;
    while (driver_name[i] && i < 31) {
        drv->name[i] = driver_name[i];
        i++;
    }
    drv->name[i] = '\0';
    
    drv->capabilities = capabilities;
    drv->max_dma_size = max_dma_size;
    drv->dma_protection_enabled = 1;
    drv->interrupt_validation = 1;
    drv->allowed_irqs = 0xFFFFFFFF;  /* All IRQs by default */
    
    hooks_state.driver_count++;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[DRIVER_HOOKS] Registered: ");
    serial_puts(SERIAL_PORT_A, driver_name);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return drv->driver_id;
}

/**
 * driver_check_dma()
 * Check DMA transfer for security violations
 * 
 * Returns: 1 if allowed, 0 if denied
 */
int driver_check_dma(uint32_t driver_id, uint32_t size)
{
    if (driver_id == 0 || driver_id > hooks_state.driver_count) {
        return 0;  /* Invalid driver */
    }
    
    driver_security_t *drv = &hooks_state.drivers[driver_id - 1];
    
    if (!drv->dma_protection_enabled) {
        return 1;  /* DMA disabled */
    }
    
    /* Check DMA size limit */
    if (size > drv->max_dma_size) {
        hooks_state.hook_blocks++;
        return 0;  /* DMA too large */
    }
    
    return 1;  /* Allowed */
}

/**
 * driver_check_interrupt()
 * Validate interrupt handler for driver
 * 
 * Returns: 1 if allowed, 0 if denied
 */
int driver_check_interrupt(uint32_t driver_id, uint32_t irq_line)
{
    if (driver_id == 0 || driver_id > hooks_state.driver_count) {
        return 0;  /* Invalid driver */
    }
    
    driver_security_t *drv = &hooks_state.drivers[driver_id - 1];
    
    if (!drv->interrupt_validation) {
        return 1;  /* Validation disabled */
    }
    
    /* Check if IRQ is in allowed mask */
    if (!(drv->allowed_irqs & (1 << (irq_line % 32)))) {
        hooks_state.hook_blocks++;
        return 0;  /* IRQ not allowed */
    }
    
    return 1;  /* Allowed */
}

/**
 * driver_set_irq_mask()
 * Configure allowed IRQ mask for driver
 */
int driver_set_irq_mask(uint32_t driver_id, uint32_t irq_mask)
{
    if (driver_id == 0 || driver_id > hooks_state.driver_count) {
        return -1;
    }
    
    driver_security_t *drv = &hooks_state.drivers[driver_id - 1];
    drv->allowed_irqs = irq_mask;
    
    return 0;
}

/**
 * driver_register_hook()
 * Register security hook for driver operation
 */
int driver_register_hook(uint32_t driver_id, hook_type_t hook_type,
                        int (*hook_func)(uint32_t, uint32_t, uint32_t))
{
    if (hooks_state.hook_count >= (DRIVER_MAX * HOOKS_PER_DRIVER)) {
        return -1;  /* Hook table full */
    }
    
    security_hook_t *hook = &hooks_state.hooks[hooks_state.hook_count];
    hook->driver_id = driver_id;
    hook->hook_type = hook_type;
    hook->hook_func = hook_func;
    
    hooks_state.hook_count++;
    
    return 0;
}

/**
 * driver_invoke_hook()
 * Invoke security hook for driver operation
 * 
 * Returns: result from hook function
 */
int driver_invoke_hook(uint32_t driver_id, hook_type_t hook_type,
                      uint32_t arg1, uint32_t arg2)
{
    int result = 1;  /* Default allow */
    
    /* Find and execute matching hooks */
    for (uint32_t i = 0; i < hooks_state.hook_count; i++) {
        security_hook_t *hook = &hooks_state.hooks[i];
        
        if (hook->driver_id == driver_id && hook->hook_type == hook_type) {
            if (hook->hook_func) {
                result = hook->hook_func(driver_id, arg1, arg2);
                hooks_state.hook_executions++;
                
                if (!result) {
                    hooks_state.hook_blocks++;
                }
                
                if (!result) {
                    return 0;  /* Early exit on block */
                }
            }
        }
    }
    
    return result;
}

/**
 * driver_enforce_capability()
 * Check driver capability for operation
 * 
 * Returns: 1 if allowed, 0 if denied
 */
int driver_enforce_capability(uint32_t driver_id, uint32_t capability_bit)
{
    if (driver_id == 0 || driver_id > hooks_state.driver_count) {
        return 0;
    }
    
    driver_security_t *drv = &hooks_state.drivers[driver_id - 1];
    
    if (!(drv->capabilities & (1 << capability_bit))) {
        hooks_state.hook_blocks++;
        return 0;  /* Capability denied */
    }
    
    return 1;  /* Capability allowed */
}

/**
 * driver_get_security_context()
 * Get security context for driver
 */
int driver_get_security_context(uint32_t driver_id, 
                               driver_security_t *out_context)
{
    if (driver_id == 0 || driver_id > hooks_state.driver_count) {
        return -1;
    }
    
    *out_context = hooks_state.drivers[driver_id - 1];
    return 0;
}

/**
 * driver_get_hook_stats()
 * Get hook execution statistics
 */
void driver_get_hook_stats(uint32_t *executions, uint32_t *blocks)
{
    if (executions) *executions = hooks_state.hook_executions;
    if (blocks) *blocks = hooks_state.hook_blocks;
}

/**
 * driver_disable_dma_protection()
 * Disable DMA protection for driver (for development)
 */
void driver_disable_dma_protection(uint32_t driver_id)
{
    if (driver_id > 0 && driver_id <= hooks_state.driver_count) {
        hooks_state.drivers[driver_id - 1].dma_protection_enabled = 0;
    }
}

/**
 * driver_disable_interrupt_validation()
 * Disable interrupt validation (for development)
 */
void driver_disable_interrupt_validation(uint32_t driver_id)
{
    if (driver_id > 0 && driver_id <= hooks_state.driver_count) {
        hooks_state.drivers[driver_id - 1].interrupt_validation = 0;
    }
}
