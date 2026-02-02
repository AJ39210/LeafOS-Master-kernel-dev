/*
 * SELeaf Driver Policy Engine
 * Advanced policy management specifically for driver security
 * 
 * Provides driver-level policy definitions, rule evaluation,
 * and fine-grained capability enforcement per driver.
 */

#include <stdint.h>
#include <string.h>
#include "../../include/SELeaf/driver_policy_engine.h"

/* Driver policy rule definition */
typedef struct {
    char driver_name[32];
    uint32_t driver_id;
    uint16_t allowed_operations;
    uint16_t denied_operations;
    uint32_t resource_limits[8];     /* Memory, CPU, IRQ, DMA, etc */
    uint8_t priority;
    uint8_t enforcement_mode;        /* 0=log, 1=warn, 2=block */
    uint32_t flags;
} driver_policy_rule_t;

/* Driver policy context */
typedef struct {
    driver_policy_rule_t rules[DRIVER_POLICY_MAX_RULES];
    uint32_t rule_count;
    uint32_t total_evals;
    uint32_t violations;
    uint32_t warnings;
} driver_policy_context_t;

static driver_policy_context_t policy_ctx = {0};

/* Predefined policy templates for common driver types */
typedef struct {
    const char *driver_type;
    uint16_t allowed_ops;
    uint16_t denied_ops;
    uint8_t enforcement_mode;
} driver_policy_template_t;

static const driver_policy_template_t templates[] = {
    /* Storage drivers - high access, all operations */
    {
        .driver_type = "ata",
        .allowed_ops = (DRIVER_OP_READ | DRIVER_OP_WRITE | DRIVER_OP_SEEK | 
                        DRIVER_OP_IRQ | DRIVER_OP_DMA | DRIVER_OP_IOCTL),
        .denied_ops = DRIVER_OP_NETWORK,
        .enforcement_mode = DRIVER_ENFORCE_BLOCK
    },
    
    /* Network drivers - network operations only */
    {
        .driver_type = "eth",
        .allowed_ops = (DRIVER_OP_NETWORK | DRIVER_OP_IRQ | DRIVER_OP_DMA),
        .denied_ops = (DRIVER_OP_SEEK | DRIVER_OP_FILE_WRITE),
        .enforcement_mode = DRIVER_ENFORCE_BLOCK
    },
    
    /* USB drivers - moderate access */
    {
        .driver_type = "usb",
        .allowed_ops = (DRIVER_OP_READ | DRIVER_OP_WRITE | DRIVER_OP_IRQ | 
                        DRIVER_OP_DMA | DRIVER_OP_IOCTL),
        .denied_ops = DRIVER_OP_NETWORK,
        .enforcement_mode = DRIVER_ENFORCE_BLOCK
    },
    
    /* GPIO drivers - minimal access */
    {
        .driver_type = "gpio",
        .allowed_ops = (DRIVER_OP_READ | DRIVER_OP_WRITE | DRIVER_OP_IOCTL),
        .denied_ops = (DRIVER_OP_NETWORK | DRIVER_OP_DMA | DRIVER_OP_SEEK),
        .enforcement_mode = DRIVER_ENFORCE_BLOCK
    },
    
    /* Character device drivers - basic I/O */
    {
        .driver_type = "char",
        .allowed_ops = (DRIVER_OP_READ | DRIVER_OP_WRITE | DRIVER_OP_IOCTL),
        .denied_ops = (DRIVER_OP_NETWORK | DRIVER_OP_DMA),
        .enforcement_mode = DRIVER_ENFORCE_WARN
    }
};

#define TEMPLATE_COUNT (sizeof(templates) / sizeof(templates[0]))

/**
 * driver_policy_init - Initialize driver policy engine
 * Initialize all structures and load default templates
 */
void driver_policy_init(void)
{
    policy_ctx.rule_count = 0;
    policy_ctx.total_evals = 0;
    policy_ctx.violations = 0;
    policy_ctx.warnings = 0;
}

/**
 * driver_policy_register - Register a driver with security policy
 * @driver_name: Name of the driver (e.g., "ata_driver")
 * @driver_id: Driver identifier
 * @operations: Bitmask of allowed operations
 * @resource_limit: Maximum resource consumption
 * @enforcement_mode: How to handle violations (log/warn/block)
 *
 * Returns: Policy rule index on success, -1 on failure
 */
int32_t driver_policy_register(const char *driver_name, uint32_t driver_id,
                               uint16_t operations, uint32_t resource_limit,
                               uint8_t enforcement_mode)
{
    if (policy_ctx.rule_count >= DRIVER_POLICY_MAX_RULES) {
        return -1;  /* Policy table full */
    }
    
    driver_policy_rule_t *rule = &policy_ctx.rules[policy_ctx.rule_count];
    
    /* Copy driver name manually */
    uint32_t i = 0;
    while (i < 31 && driver_name[i] != '\0') {
        rule->driver_name[i] = driver_name[i];
        i++;
    }
    rule->driver_name[i] = '\0';
    
    rule->driver_id = driver_id;
    rule->allowed_operations = operations;
    rule->denied_operations = ~operations;
    rule->resource_limits[0] = resource_limit;
    rule->enforcement_mode = enforcement_mode;
    rule->priority = DRIVER_POLICY_PRIORITY_NORMAL;
    rule->flags = DRIVER_POLICY_FLAG_ACTIVE;
    
    return (int32_t)policy_ctx.rule_count++;
}

/**
 * driver_policy_from_template - Create policy from driver type template
 * @driver_name: Driver name (e.g., "ata_driver")
 * @driver_type: Type category (e.g., "ata", "eth", "usb")
 * @driver_id: Driver ID
 *
 * Returns: Policy rule index on success, -1 on failure
 */
int32_t driver_policy_from_template(const char *driver_name,
                                    const char *driver_type,
                                    uint32_t driver_id)
{
    uint32_t i;
    
    /* Find matching template */
    for (i = 0; i < TEMPLATE_COUNT; i++) {
        /* Compare strings manually */
        uint32_t j = 0;
        uint8_t match = 1;
        while (j < 16 && (templates[i].driver_type[j] != '\0' || driver_type[j] != '\0')) {
            if (templates[i].driver_type[j] != driver_type[j]) {
                match = 0;
                break;
            }
            j++;
        }
        
        if (match) {
            return driver_policy_register(
                driver_name,
                driver_id,
                templates[i].allowed_ops,
                DRIVER_POLICY_DEFAULT_RESOURCE_LIMIT,
                templates[i].enforcement_mode
            );
        }
    }
    
    return -1;  /* Template not found */
}

/**
 * driver_policy_evaluate - Evaluate if operation is allowed
 * @driver_id: Driver making the request
 * @operation: Operation being requested (DRIVER_OP_*)
 * @resource_size: Size of resource being accessed
 *
 * Returns: DRIVER_POLICY_ALLOW, DRIVER_POLICY_DENY, or DRIVER_POLICY_WARN
 */
uint8_t driver_policy_evaluate(uint32_t driver_id, uint16_t operation,
                               uint32_t resource_size)
{
    uint32_t i;
    driver_policy_rule_t *rule;
    
    policy_ctx.total_evals++;
    
    /* Find rule for this driver */
    for (i = 0; i < policy_ctx.rule_count; i++) {
        rule = &policy_ctx.rules[i];
        
        if (rule->driver_id != driver_id)
            continue;
        
        if (!(rule->flags & DRIVER_POLICY_FLAG_ACTIVE))
            continue;
        
        /* Check if operation is denied */
        if (operation & rule->denied_operations) {
            policy_ctx.violations++;
            
            if (rule->enforcement_mode == DRIVER_ENFORCE_BLOCK)
                return DRIVER_POLICY_DENY;
            else if (rule->enforcement_mode == DRIVER_ENFORCE_WARN) {
                policy_ctx.warnings++;
                return DRIVER_POLICY_WARN;
            }
            return DRIVER_POLICY_ALLOW;  /* Log only */
        }
        
        /* Check if operation is allowed */
        if (operation & rule->allowed_operations) {
            /* Check resource limits */
            if (resource_size > rule->resource_limits[0]) {
                policy_ctx.violations++;
                
                if (rule->enforcement_mode == DRIVER_ENFORCE_BLOCK)
                    return DRIVER_POLICY_DENY;
            }
            
            return DRIVER_POLICY_ALLOW;
        }
    }
    
    /* No rule found - default deny */
    policy_ctx.violations++;
    return DRIVER_POLICY_DENY;
}

/**
 * driver_policy_set_enforcement - Change enforcement mode for driver
 * @driver_id: Driver ID
 * @mode: New enforcement mode (DRIVER_ENFORCE_LOG/WARN/BLOCK)
 *
 * Returns: 0 on success, -1 on failure
 */
int32_t driver_policy_set_enforcement(uint32_t driver_id, uint8_t mode)
{
    uint32_t i;
    
    for (i = 0; i < policy_ctx.rule_count; i++) {
        if (policy_ctx.rules[i].driver_id == driver_id) {
            policy_ctx.rules[i].enforcement_mode = mode;
            return 0;
        }
    }
    
    return -1;
}

/**
 * driver_policy_update_limits - Update resource limits for driver
 * @driver_id: Driver ID
 * @resource_type: Resource index (0=memory, 1=irq, 2=dma, etc)
 * @limit: New limit value
 *
 * Returns: 0 on success, -1 on failure
 */
int32_t driver_policy_update_limits(uint32_t driver_id, uint8_t resource_type,
                                    uint32_t limit)
{
    uint32_t i;
    
    if (resource_type >= 8)
        return -1;
    
    for (i = 0; i < policy_ctx.rule_count; i++) {
        if (policy_ctx.rules[i].driver_id == driver_id) {
            policy_ctx.rules[i].resource_limits[resource_type] = limit;
            return 0;
        }
    }
    
    return -1;
}

/**
 * driver_policy_enable - Enable policy for driver
 * @driver_id: Driver ID
 *
 * Returns: 0 on success, -1 on failure
 */
int32_t driver_policy_enable(uint32_t driver_id)
{
    uint32_t i;
    
    for (i = 0; i < policy_ctx.rule_count; i++) {
        if (policy_ctx.rules[i].driver_id == driver_id) {
            policy_ctx.rules[i].flags |= DRIVER_POLICY_FLAG_ACTIVE;
            return 0;
        }
    }
    
    return -1;
}

/**
 * driver_policy_disable - Disable policy for driver
 * @driver_id: Driver ID
 *
 * Returns: 0 on success, -1 on failure
 */
int32_t driver_policy_disable(uint32_t driver_id)
{
    uint32_t i;
    
    for (i = 0; i < policy_ctx.rule_count; i++) {
        if (policy_ctx.rules[i].driver_id == driver_id) {
            policy_ctx.rules[i].flags &= ~DRIVER_POLICY_FLAG_ACTIVE;
            return 0;
        }
    }
    
    return -1;
}

/**
 * driver_policy_get_stats - Get policy evaluation statistics
 * @stats: Output statistics structure
 */
void driver_policy_get_stats(driver_policy_stats_t *stats)
{
    stats->total_rules = policy_ctx.rule_count;
    stats->total_evaluations = policy_ctx.total_evals;
    stats->total_violations = policy_ctx.violations;
    stats->total_warnings = policy_ctx.warnings;
    stats->active_drivers = 0;
    
    uint32_t i;
    for (i = 0; i < policy_ctx.rule_count; i++) {
        if (policy_ctx.rules[i].flags & DRIVER_POLICY_FLAG_ACTIVE)
            stats->active_drivers++;
    }
}

/**
 * driver_policy_dump - Display all driver policies (debugging)
 */
void driver_policy_dump(void)
{
    uint32_t i;
    driver_policy_rule_t *rule;
    
    /* Would output to serial console in real implementation */
    for (i = 0; i < policy_ctx.rule_count; i++) {
        rule = &policy_ctx.rules[i];
        
        if (!(rule->flags & DRIVER_POLICY_FLAG_ACTIVE))
            continue;
        
        /* Format: Driver | Allowed Ops | Enforcement Mode | Resources */
        /* Example output: ata_driver | READ|WRITE|DMA | BLOCK | 1MB */
    }
}
