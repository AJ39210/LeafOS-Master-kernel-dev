/**
 * include/SELeaf/sandbox.h
 * 
 * SELeaf Sandbox Mode
 */

#ifndef SELEAF_SANDBOX_H
#define SELEAF_SANDBOX_H

#include <stdint.h>

/* Sandbox capabilities (operations allowed) */
#define SANDBOX_OP_FILE_READ        (1 << 0)
#define SANDBOX_OP_FILE_WRITE       (1 << 1)
#define SANDBOX_OP_FILE_EXECUTE     (1 << 2)
#define SANDBOX_OP_NETWORK_SEND     (1 << 3)
#define SANDBOX_OP_NETWORK_RECV     (1 << 4)
#define SANDBOX_OP_DEVICE_READ      (1 << 5)
#define SANDBOX_OP_DEVICE_WRITE     (1 << 6)
#define SANDBOX_OP_PROCESS_FORK     (1 << 7)
#define SANDBOX_OP_PROCESS_EXEC     (1 << 8)
#define SANDBOX_OP_MEMORY_ALLOCATE  (1 << 9)

/* Function declarations */
int sandbox_init(void);
int sandbox_enable_process(uint32_t process_id, uint32_t capabilities,
                          uint32_t max_memory_kb);
int sandbox_disable_process(uint32_t process_id);
int sandbox_check_operation(uint32_t process_id, uint32_t operation);
int sandbox_check_memory(uint32_t process_id, uint32_t size_kb,
                        uint32_t current_kb);
int sandbox_check_file_size(uint32_t process_id, uint32_t size_kb);
int sandbox_set_max_open_files(uint32_t process_id, uint32_t max);
int sandbox_set_device_access(uint32_t process_id, uint32_t device_mask);
int sandbox_is_active(uint32_t process_id);
void sandbox_get_stats(uint32_t *total_sandboxes, uint32_t *escape_attempts,
                      uint32_t *policy_violations);
void sandbox_reset_stats(void);

#endif /* SELEAF_SANDBOX_H */
