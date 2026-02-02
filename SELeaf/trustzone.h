/*
 * SELeaf TrustZone Header
 */

#ifndef SELEAF_TRUSTZONE_H
#define SELEAF_TRUSTZONE_H

#include <stdint.h>

/* Maximum numbers */
#define TRUSTZONE_MAX_SERVICES          16      /* Secure services */
#define TRUSTZONE_MAX_TASKS             32      /* Secure tasks */

/* TrustZone Version */
#define TRUSTZONE_MONITOR_VERSION       0x00010002

/* Service IDs */
#define TRUSTZONE_SERVICE_ID_CRYPTO     0x01
#define TRUSTZONE_SERVICE_ID_ATTESTATION 0x02
#define TRUSTZONE_SERVICE_ID_KEYSTORE   0x03
#define TRUSTZONE_SERVICE_ID_SENSOR     0x04

/* Service Types */
#define TRUSTZONE_SERVICE_CRYPTO        0x01
#define TRUSTZONE_SERVICE_ATTESTATION   0x02
#define TRUSTZONE_SERVICE_KEYSTORE      0x03
#define TRUSTZONE_SERVICE_SENSOR        0x04

/* Service Permissions */
#define TRUSTZONE_PERM_CALL             (1 << 0)
#define TRUSTZONE_PERM_READ             (1 << 1)
#define TRUSTZONE_PERM_WRITE            (1 << 2)
#define TRUSTZONE_PERM_MEMORY_ACCESS    (1 << 3)

/* Task Status */
#define TRUSTZONE_TASK_PENDING          0
#define TRUSTZONE_TASK_RUNNING          1
#define TRUSTZONE_TASK_COMPLETED        2
#define TRUSTZONE_TASK_ERROR            3

/* Task IDs */
#define TRUSTZONE_TASK_ID_BASE          0xA0000000

/* SMC Return Values */
#define TRUSTZONE_SMC_SUCCESS           0
#define TRUSTZONE_SMC_ERROR             -1

/* Status Structure */
typedef struct {
    uint32_t monitor_version;
    uint8_t trustzone_active;
    uint8_t secure_world_active;
    uint32_t total_services;
    uint32_t total_tasks;
    uint32_t total_smc_calls;
    uint32_t smc_errors;
    uint32_t secure_interrupts;
    uint32_t normal_interrupts;
} trustzone_status_t;

/* Function declarations */
int32_t trustzone_init(void);

int32_t trustzone_register_service(uint32_t service_id, const char *service_name,
                                   uint8_t service_type, uint32_t permissions);

int32_t trustzone_create_task(uint32_t service_id, uint32_t entry_point,
                              uint32_t secure_memory_size);

int32_t trustzone_smc_call(uint32_t service_id, uint32_t command,
                          uint32_t param1, uint32_t param2);

int32_t trustzone_switch_to_secure(uint32_t task_id);

int32_t trustzone_map_shared_memory(uint32_t address, uint32_t size,
                                   uint32_t permissions);

int32_t trustzone_isolate_memory(uint32_t address, uint32_t size);

int32_t trustzone_interrupt_handler(uint32_t interrupt_id);

void trustzone_get_status(trustzone_status_t *status);

int32_t trustzone_enable_fiq_security(uint8_t fiq_number);

int32_t trustzone_attestation_quote(uint32_t nonce, uint8_t *quote_data,
                                   uint32_t max_size);

int32_t trustzone_seal_to_world(const uint8_t *secret, uint32_t secret_size);

#endif /* SELEAF_TRUSTZONE_H */
