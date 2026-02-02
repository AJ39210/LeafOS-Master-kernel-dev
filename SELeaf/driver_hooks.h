/**
 * include/SELeaf/driver_hooks.h
 */

#ifndef SELEAF_DRIVER_HOOKS_H
#define SELEAF_DRIVER_HOOKS_H

#include <stdint.h>

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

typedef struct {
    uint32_t driver_id;
    char name[32];
    uint32_t capabilities;
    uint32_t max_dma_size;
} driver_security_t;

int driver_hooks_init(void);
uint32_t driver_register_security(const char *driver_name, uint32_t capabilities,
                                 uint32_t max_dma_size);
int driver_check_dma(uint32_t driver_id, uint32_t size);
int driver_check_interrupt(uint32_t driver_id, uint32_t irq_line);
int driver_set_irq_mask(uint32_t driver_id, uint32_t irq_mask);
int driver_register_hook(uint32_t driver_id, hook_type_t hook_type,
                        int (*hook_func)(uint32_t, uint32_t, uint32_t));
int driver_invoke_hook(uint32_t driver_id, hook_type_t hook_type,
                      uint32_t arg1, uint32_t arg2);
int driver_enforce_capability(uint32_t driver_id, uint32_t capability_bit);
int driver_get_security_context(uint32_t driver_id, driver_security_t *out_context);
void driver_get_hook_stats(uint32_t *executions, uint32_t *blocks);
void driver_disable_dma_protection(uint32_t driver_id);
void driver_disable_interrupt_validation(uint32_t driver_id);

#endif
