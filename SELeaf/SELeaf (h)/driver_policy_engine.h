/*
 * SELeaf Driver Policy Engine Header
 */

#ifndef SELEAF_DRIVER_POLICY_ENGINE_H
#define SELEAF_DRIVER_POLICY_ENGINE_H

#include <stdint.h>

/* Maximum number of driver policies */
#define DRIVER_POLICY_MAX_RULES         32

/* Default resource limit for drivers (1MB) */
#define DRIVER_POLICY_DEFAULT_RESOURCE_LIMIT    (1024 * 1024)

/* Driver operations bitmask */
#define DRIVER_OP_READ                  (1 << 0)
#define DRIVER_OP_WRITE                 (1 << 1)
#define DRIVER_OP_SEEK                  (1 << 2)
#define DRIVER_OP_IRQ                   (1 << 3)
#define DRIVER_OP_DMA                   (1 << 4)
#define DRIVER_OP_IOCTL                 (1 << 5)
#define DRIVER_OP_NETWORK               (1 << 6)
#define DRIVER_OP_FILE_WRITE            (1 << 7)

/* Policy enforcement modes */
#define DRIVER_ENFORCE_LOG              0    /* Log only */
#define DRIVER_ENFORCE_WARN             1    /* Log and warn */
#define DRIVER_ENFORCE_BLOCK            2    /* Block violations */

/* Policy decision results */
#define DRIVER_POLICY_ALLOW             1
#define DRIVER_POLICY_WARN              2
#define DRIVER_POLICY_DENY              0

/* Policy priority levels */
#define DRIVER_POLICY_PRIORITY_LOW      0
#define DRIVER_POLICY_PRIORITY_NORMAL   1
#define DRIVER_POLICY_PRIORITY_HIGH     2

/* Policy flags */
#define DRIVER_POLICY_FLAG_ACTIVE       (1 << 0)
#define DRIVER_POLICY_FLAG_LOCKED       (1 << 1)
#define DRIVER_POLICY_FLAG_AUDIT_ALL    (1 << 2)

/* Statistics structure */
typedef struct {
    uint32_t total_rules;
    uint32_t total_evaluations;
    uint32_t total_violations;
    uint32_t total_warnings;
    uint32_t active_drivers;
} driver_policy_stats_t;

/* Function declarations */
void driver_policy_init(void);

int32_t driver_policy_register(const char *driver_name, uint32_t driver_id,
                               uint16_t operations, uint32_t resource_limit,
                               uint8_t enforcement_mode);

int32_t driver_policy_from_template(const char *driver_name,
                                    const char *driver_type,
                                    uint32_t driver_id);

uint8_t driver_policy_evaluate(uint32_t driver_id, uint16_t operation,
                               uint32_t resource_size);

int32_t driver_policy_set_enforcement(uint32_t driver_id, uint8_t mode);

int32_t driver_policy_update_limits(uint32_t driver_id, uint8_t resource_type,
                                    uint32_t limit);

int32_t driver_policy_enable(uint32_t driver_id);

int32_t driver_policy_disable(uint32_t driver_id);

void driver_policy_get_stats(driver_policy_stats_t *stats);

void driver_policy_dump(void);

#endif /* SELEAF_DRIVER_POLICY_ENGINE_H */
