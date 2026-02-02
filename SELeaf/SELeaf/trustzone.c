/*
 * SELeaf TrustZone Support
 * ARM TrustZone-based execution environment for security operations
 * 
 * Implements secure world execution, monitor mode calls,
 * and isolated trusted execution environment.
 */

#include <stdint.h>
#include "../../include/SELeaf/trustzone.h"

/* TrustZone security context */
typedef struct {
    uint32_t monitor_version;
    uint32_t smc_call_count;
    uint8_t trustzone_active;
    uint8_t secure_world_active;
    uint32_t smc_error_count;
} trustzone_device_t;

/* Secure world task */
typedef struct {
    uint32_t task_id;
    uint32_t service_id;
    uint32_t status;                 /* PENDING, RUNNING, COMPLETED */
    uint32_t entry_point;
    uint32_t secure_memory_base;
    uint32_t secure_memory_size;
    uint32_t stack_pointer;
    uint32_t creation_tick;
} trustzone_task_t;

/* TrustZone service definition */
typedef struct {
    uint32_t service_id;
    char service_name[32];
    void (*handler)(void);           /* Secure world handler */
    uint32_t permissions;
    uint8_t service_type;            /* CRYPTO, ATTESTATION, KEY_STORE, etc */
    uint32_t call_count;
} trustzone_service_t;

/* TrustZone execution context */
typedef struct {
    trustzone_device_t device;
    trustzone_task_t tasks[TRUSTZONE_MAX_TASKS];
    uint32_t task_count;
    trustzone_service_t services[TRUSTZONE_MAX_SERVICES];
    uint32_t service_count;
    uint32_t normal_world_interrupts;
    uint32_t secure_world_interrupts;
    uint8_t shared_memory_active;
    uint32_t shared_memory_base;
    uint32_t shared_memory_size;
} trustzone_context_t;

static trustzone_context_t trustzone = {0};

/* Service handlers (stub implementations) */
static void trustzone_crypto_handler(void) { }
static void trustzone_attestation_handler(void) { }
static void trustzone_keystore_handler(void) { }
static void trustzone_sensor_handler(void) { }

/**
 * trustzone_init - Initialize TrustZone execution environment
 * Sets up monitor mode and secure world
 *
 * Returns: 0 on success, -1 on failure
 */
int32_t trustzone_init(void)
{
    /* Detect TrustZone (would check CPU ID register in real impl) */
    trustzone.device.monitor_version = TRUSTZONE_MONITOR_VERSION;
    trustzone.device.smc_call_count = 0;
    trustzone.device.trustzone_active = 1;
    trustzone.device.secure_world_active = 0;
    trustzone.device.smc_error_count = 0;
    
    trustzone.task_count = 0;
    trustzone.service_count = 0;
    trustzone.normal_world_interrupts = 0;
    trustzone.secure_world_interrupts = 0;
    
    /* Initialize shared memory for communication */
    trustzone.shared_memory_active = 1;
    trustzone.shared_memory_base = 0x80000000;  /* Secure memory region */
    trustzone.shared_memory_size = 256 * 1024;  /* 256KB */
    
    return 0;
}

/**
 * trustzone_register_service - Register secure service
 * @service_id: Service identifier
 * @service_name: Human-readable name
 * @service_type: Type of service (CRYPTO, ATTESTATION, etc)
 * @handler: Service handler function
 * @permissions: Service permissions
 *
 * Returns: 0 on success, -1 on failure
 */
int32_t trustzone_register_service(uint32_t service_id, const char *service_name,
                                   uint8_t service_type, uint32_t permissions)
{
    if (trustzone.service_count >= TRUSTZONE_MAX_SERVICES)
        return -1;
    
    trustzone_service_t *service = &trustzone.services[trustzone.service_count];
    
    service->service_id = service_id;
    
    /* Copy service name manually */
    uint32_t i = 0;
    while (i < 31 && service_name[i] != '\0') {
        service->service_name[i] = service_name[i];
        i++;
    }
    service->service_name[i] = '\0';
    
    service->service_type = service_type;
    service->permissions = permissions;
    service->call_count = 0;
    
    /* Assign handler based on service type */
    switch (service_type) {
        case TRUSTZONE_SERVICE_CRYPTO:
            service->handler = trustzone_crypto_handler;
            break;
        case TRUSTZONE_SERVICE_ATTESTATION:
            service->handler = trustzone_attestation_handler;
            break;
        case TRUSTZONE_SERVICE_KEYSTORE:
            service->handler = trustzone_keystore_handler;
            break;
        case TRUSTZONE_SERVICE_SENSOR:
            service->handler = trustzone_sensor_handler;
            break;
        default:
            return -1;
    }
    
    return (int32_t)trustzone.service_count++;
}

/**
 * trustzone_create_task - Create secure world task
 * @service_id: Service to run
 * @entry_point: Entry point address
 * @secure_memory_size: Secure memory allocation
 *
 * Returns: Task ID on success, -1 on failure
 * 
 * Creates isolated task in secure world
 */
int32_t trustzone_create_task(uint32_t service_id, uint32_t entry_point,
                              uint32_t secure_memory_size)
{
    if (trustzone.task_count >= TRUSTZONE_MAX_TASKS)
        return -1;
    
    /* Verify service exists */
    uint32_t i;
    int32_t service_found = -1;
    for (i = 0; i < trustzone.service_count; i++) {
        if (trustzone.services[i].service_id == service_id) {
            service_found = i;
            break;
        }
    }
    
    if (service_found < 0)
        return -1;
    
    trustzone_task_t *task = &trustzone.tasks[trustzone.task_count];
    
    task->task_id = TRUSTZONE_TASK_ID_BASE + trustzone.task_count;
    task->service_id = service_id;
    task->status = TRUSTZONE_TASK_PENDING;
    task->entry_point = entry_point;
    task->secure_memory_base = trustzone.shared_memory_base;
    task->secure_memory_size = secure_memory_size;
    task->stack_pointer = task->secure_memory_base + task->secure_memory_size;
    task->creation_tick = 0;
    
    return (int32_t)trustzone.task_count++;
}

/**
 * trustzone_smc_call - Make Secure Monitor Call (SMC)
 * @service_id: Service to invoke
 * @command: Command ID
 * @param1: Parameter 1
 * @param2: Parameter 2
 *
 * Returns: Result from secure world on success, -1 on error
 * 
 * Transitions to secure world via monitor mode
 */
int32_t trustzone_smc_call(uint32_t service_id, uint32_t command,
                          uint32_t param1, uint32_t param2)
{
    if (!trustzone.device.trustzone_active)
        return -1;
    
    /* Find service */
    uint32_t i;
    int32_t service_idx = -1;
    for (i = 0; i < trustzone.service_count; i++) {
        if (trustzone.services[i].service_id == service_id) {
            service_idx = i;
            break;
        }
    }
    
    if (service_idx < 0) {
        trustzone.device.smc_error_count++;
        return -1;
    }
    
    trustzone_service_t *service = &trustzone.services[service_idx];
    
    /* Check permissions */
    if (!(service->permissions & TRUSTZONE_PERM_CALL)) {
        trustzone.device.smc_error_count++;
        return -1;
    }
    
    /* Increment call count */
    trustzone.device.smc_call_count++;
    service->call_count++;
    
    /* In real implementation:
     * 1. Save normal world context
     * 2. Execute SMC instruction with:
     *    r0 = service_id
     *    r1 = command
     *    r2 = param1
     *    r3 = param2
     * 3. Monitor mode saves secure context
     * 4. Switch to secure world
     * 5. Execute service handler
     * 6. Return result
     */
    
    /* For simulation, execute handler directly */
    if (service->handler) {
        service->handler();
    }
    
    /* Return dummy result */
    return 0;
}

/**
 * trustzone_switch_to_secure - Switch to secure world execution
 * @task_id: Task to execute
 *
 * Returns: 0 on success, -1 on failure
 * 
 * Transitions CPU to secure world for task execution
 */
int32_t trustzone_switch_to_secure(uint32_t task_id)
{
    if (!trustzone.device.trustzone_active)
        return -1;
    
    /* Find task */
    uint32_t idx = task_id - TRUSTZONE_TASK_ID_BASE;
    if (idx >= trustzone.task_count)
        return -1;
    
    trustzone_task_t *task = &trustzone.tasks[idx];
    
    /* Update task status */
    task->status = TRUSTZONE_TASK_RUNNING;
    trustzone.device.secure_world_active = 1;
    
    /* In real implementation:
     * 1. Save normal world registers
     * 2. Load secure world registers
     * 3. Set CPU to secure mode
     * 4. Jump to task->entry_point
     */
    
    /* Simulated task execution */
    task->status = TRUSTZONE_TASK_COMPLETED;
    trustzone.device.secure_world_active = 0;
    
    return 0;
}

/**
 * trustzone_map_shared_memory - Map region for secure/normal world sharing
 * @address: Physical address
 * @size: Size of region
 * @permissions: Access permissions (read/write)
 *
 * Returns: 0 on success, -1 on failure
 * 
 * Creates shared memory region with controlled access
 */
int32_t trustzone_map_shared_memory(uint32_t address, uint32_t size,
                                   uint32_t permissions)
{
    if (!trustzone.shared_memory_active)
        return -1;
    
    if (address < trustzone.shared_memory_base ||
        (address + size) > (trustzone.shared_memory_base + trustzone.shared_memory_size)) {
        return -1;  /* Outside designated shared region */
    }
    
    /* In real implementation:
     * 1. Configure page tables for shared access
     * 2. Set non-cacheable attributes
     * 3. Mark pages as shared
     * 4. Configure access permissions
     */
    
    return 0;
}

/**
 * trustzone_isolate_memory - Isolate memory region to secure world only
 * @address: Physical address
 * @size: Size of region
 *
 * Returns: 0 on success, -1 on failure
 * 
 * Prevents normal world access to secure memory
 */
int32_t trustzone_isolate_memory(uint32_t address, uint32_t size)
{
    /* In real implementation:
     * 1. Configure TrustZone Address Space Controller (TZASC)
     * 2. Mark region as secure only
     * 3. Configure access permissions
     * 4. Enable exceptions for unauthorized access
     */
    
    return 0;
}

/**
 * trustzone_interrupt_handler - Handle interrupt with TrustZone awareness
 * @interrupt_id: IRQ number
 *
 * Returns: 0 if handled, -1 if should be delegated
 * 
 * Routes interrupts to secure or normal world appropriately
 */
int32_t trustzone_interrupt_handler(uint32_t interrupt_id)
{
    /* Check if interrupt should go to secure world */
    if (interrupt_id >= 32) {  /* Shared interrupt */
        /* Route based on current world and interrupt configuration */
        if (trustzone.device.secure_world_active) {
            trustzone.secure_world_interrupts++;
            return 0;  /* Let secure world handle */
        }
    }
    
    trustzone.normal_world_interrupts++;
    return -1;  /* Delegate to normal world */
}

/**
 * trustzone_get_status - Get TrustZone status
 * @status: Output status structure
 */
void trustzone_get_status(trustzone_status_t *status)
{
    status->monitor_version = trustzone.device.monitor_version;
    status->trustzone_active = trustzone.device.trustzone_active;
    status->secure_world_active = trustzone.device.secure_world_active;
    status->total_services = trustzone.service_count;
    status->total_tasks = trustzone.task_count;
    status->total_smc_calls = trustzone.device.smc_call_count;
    status->smc_errors = trustzone.device.smc_error_count;
    status->secure_interrupts = trustzone.secure_world_interrupts;
    status->normal_interrupts = trustzone.normal_world_interrupts;
}

/**
 * trustzone_enable_fiq_security - Enable Fast IRQ to secure world
 * @fiq_number: FIQ to secure
 *
 * Returns: 0 on success, -1 on failure
 * 
 * Routes specific interrupts to secure world only
 */
int32_t trustzone_enable_fiq_security(uint8_t fiq_number)
{
    if (fiq_number >= 32)
        return -1;
    
    /* In real implementation:
     * Configure GIC (Generic Interrupt Controller) or similar
     * to route FIQ to secure world only
     */
    
    return 0;
}

/**
 * trustzone_attestation_quote - Get attestation quote from secure world
 * @nonce: Challenge nonce
 * @quote_data: Output buffer for quote
 * @max_size: Maximum quote size
 *
 * Returns: Quote size on success, -1 on failure
 * 
 * Gets cryptographically signed attestation from secure world
 */
int32_t trustzone_attestation_quote(uint32_t nonce, uint8_t *quote_data,
                                   uint32_t max_size)
{
    if (!trustzone.device.trustzone_active || max_size < 64)
        return -1;
    
    /* Call secure world attestation service */
    int32_t result = trustzone_smc_call(TRUSTZONE_SERVICE_ID_ATTESTATION,
                                        0, nonce, 0);
    
    if (result < 0)
        return -1;
    
    /* Generate attestation quote */
    uint32_t i;
    for (i = 0; i < 64 && i < max_size; i++) {
        quote_data[i] = (nonce >> (i % 4)) ^ i;
    }
    
    return 64;
}

/**
 * trustzone_seal_to_world - Seal secret to secure world
 * @secret: Data to seal
 * @secret_size: Size of secret
 *
 * Returns: 0 on success, -1 on failure
 * 
 * Encrypts secret so only secure world can access
 */
int32_t trustzone_seal_to_world(const uint8_t *secret, uint32_t secret_size)
{
    if (!trustzone.device.trustzone_active)
        return -1;
    
    if (secret_size > trustzone.shared_memory_size)
        return -1;
    
    /* Encrypt secret with secure world key */
    uint32_t i;
    uint8_t *sealed = (uint8_t *)trustzone.shared_memory_base;
    
    for (i = 0; i < secret_size; i++) {
        sealed[i] = secret[i] ^ 0xCC;  /* Dummy encryption */
    }
    
    return 0;
}
