/**
 * kernel/SELeaf/sandbox.c
 * 
 * SELeaf Sandbox Mode
 * 
 * Provides:
 * - Process sandboxing
 * - Capability restriction
 * - Resource limiting
 * - Escape detection
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Sandbox state */
typedef struct {
    uint32_t process_id;
    uint32_t capabilities;      /* Allowed capabilities (bitmask) */
    uint32_t max_memory;        /* Max memory in KB */
    uint32_t max_file_size;     /* Max file size in KB */
    uint32_t max_open_files;    /* Max open file descriptors */
    uint32_t allowed_syscalls;  /* Syscall whitelist (bitmask) */
    uint32_t allowed_devices;   /* Device access (bitmask) */
    int enabled;
} sandbox_t;

/* Sandbox operations (bitmasks) */
#define OP_FILE_READ        (1 << 0)
#define OP_FILE_WRITE       (1 << 1)
#define OP_FILE_EXECUTE     (1 << 2)
#define OP_NETWORK_SEND     (1 << 3)
#define OP_NETWORK_RECV     (1 << 4)
#define OP_DEVICE_READ      (1 << 5)
#define OP_DEVICE_WRITE     (1 << 6)
#define OP_PROCESS_FORK     (1 << 7)
#define OP_PROCESS_EXEC     (1 << 8)
#define OP_MEMORY_ALLOCATE  (1 << 9)

#define SANDBOX_MAX         32

typedef struct {
    sandbox_t sandboxes[SANDBOX_MAX];
    uint32_t sandbox_count;
    
    uint32_t escape_attempts;   /* Total escape attempts blocked */
    uint32_t policy_violations; /* Total policy violations logged */
} sandbox_state_t;

static sandbox_state_t sandbox_state = {0};

/**
 * sandbox_init()
 * Initialize sandbox system
 */
int sandbox_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SANDBOX] Sandbox system initialized\n");
    #endif
    
    sandbox_state.sandbox_count = 0;
    sandbox_state.escape_attempts = 0;
    sandbox_state.policy_violations = 0;
    
    return 0;
}

/**
 * sandbox_enable_process()
 * Enable sandboxing for a process
 */
int sandbox_enable_process(uint32_t process_id, uint32_t capabilities,
                           uint32_t max_memory_kb)
{
    if (sandbox_state.sandbox_count >= SANDBOX_MAX) {
        return -1;  /* Error: table full */
    }
    
    /* Check if already sandboxed */
    for (uint32_t i = 0; i < sandbox_state.sandbox_count; i++) {
        if (sandbox_state.sandboxes[i].process_id == process_id) {
            sandbox_state.sandboxes[i].capabilities = capabilities;
            sandbox_state.sandboxes[i].max_memory = max_memory_kb;
            sandbox_state.sandboxes[i].enabled = 1;
            return 0;  /* Updated existing */
        }
    }
    
    /* Create new sandbox */
    sandbox_t *sb = &sandbox_state.sandboxes[sandbox_state.sandbox_count];
    sb->process_id = process_id;
    sb->capabilities = capabilities;
    sb->max_memory = max_memory_kb;
    sb->max_file_size = 0x7FFFFFFF;  /* Unlimited by default */
    sb->max_open_files = 32;
    sb->allowed_syscalls = 0xFFFFFFFF;  /* All by default */
    sb->allowed_devices = 0;
    sb->enabled = 1;
    
    sandbox_state.sandbox_count++;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SANDBOX] Enabled for PID ");
    /* Convert PID to hex string */
    char hex_buf[16];
    int i = 0;
    uint32_t temp = process_id;
    while (temp > 0 && i < 15) {
        uint8_t digit = temp % 16;
        hex_buf[14-i] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        temp /= 16;
        i++;
    }
    hex_buf[15] = '\0';
    if (i > 0) {
        serial_puts(SERIAL_PORT_A, &hex_buf[15-i]);
    }
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return 0;
}

/**
 * sandbox_disable_process()
 * Disable sandboxing for a process
 */
int sandbox_disable_process(uint32_t process_id)
{
    for (uint32_t i = 0; i < sandbox_state.sandbox_count; i++) {
        if (sandbox_state.sandboxes[i].process_id == process_id) {
            sandbox_state.sandboxes[i].enabled = 0;
            return 0;
        }
    }
    
    return -1;  /* Not found */
}

/**
 * sandbox_check_operation()
 * Check if operation is allowed in sandbox
 * 
 * Returns: 1 if allowed, 0 if denied (escape attempt)
 */
int sandbox_check_operation(uint32_t process_id, uint32_t operation)
{
    for (uint32_t i = 0; i < sandbox_state.sandbox_count; i++) {
        if (sandbox_state.sandboxes[i].process_id == process_id) {
            sandbox_t *sb = &sandbox_state.sandboxes[i];
            
            if (!sb->enabled) {
                return 1;  /* Sandbox disabled */
            }
            
            /* Check operation against capabilities */
            if (!(sb->capabilities & (1 << operation))) {
                sb = (sandbox_t *)sb;  /* Suppress unused warning */
                sandbox_state.escape_attempts++;
                
                #ifdef CONFIG_SERIAL_DRIVER
                serial_puts(SERIAL_PORT_A, "[SANDBOX] ESCAPE: PID ");
                /* Convert PID to hex */
                char hex_buf1[16];
                int i1 = 0;
                uint32_t temp1 = process_id;
                while (temp1 > 0 && i1 < 15) {
                    uint8_t digit = temp1 % 16;
                    hex_buf1[14-i1] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
                    temp1 /= 16;
                    i1++;
                }
                hex_buf1[15] = '\0';
                if (i1 > 0) {
                    serial_puts(SERIAL_PORT_A, &hex_buf1[15-i1]);
                }
                serial_puts(SERIAL_PORT_A, " op=");
                /* Convert operation to hex */
                char hex_buf2[16];
                int i2 = 0;
                uint32_t temp2 = operation;
                while (temp2 > 0 && i2 < 15) {
                    uint8_t digit = temp2 % 16;
                    hex_buf2[14-i2] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
                    temp2 /= 16;
                    i2++;
                }
                hex_buf2[15] = '\0';
                if (i2 > 0) {
                    serial_puts(SERIAL_PORT_A, &hex_buf2[15-i2]);
                }
                serial_puts(SERIAL_PORT_A, "\n");
                #endif
                
                return 0;  /* Denied */
            }
            
            return 1;  /* Allowed */
        }
    }
    
    return 1;  /* No sandbox = allow */
}

/**
 * sandbox_check_memory()
 * Check memory allocation against limit
 * 
 * Returns: 1 if allowed, 0 if would exceed limit
 */
int sandbox_check_memory(uint32_t process_id, uint32_t size_kb, uint32_t current_kb)
{
    for (uint32_t i = 0; i < sandbox_state.sandbox_count; i++) {
        if (sandbox_state.sandboxes[i].process_id == process_id) {
            sandbox_t *sb = &sandbox_state.sandboxes[i];
            
            if (!sb->enabled) {
                return 1;
            }
            
            if ((current_kb + size_kb) > sb->max_memory) {
                sandbox_state.policy_violations++;
                return 0;  /* Would exceed limit */
            }
            
            return 1;
        }
    }
    
    return 1;  /* No sandbox = allow */
}

/**
 * sandbox_check_file_size()
 * Check file size against limit
 * 
 * Returns: 1 if allowed, 0 if exceeds limit
 */
int sandbox_check_file_size(uint32_t process_id, uint32_t size_kb)
{
    for (uint32_t i = 0; i < sandbox_state.sandbox_count; i++) {
        if (sandbox_state.sandboxes[i].process_id == process_id) {
            sandbox_t *sb = &sandbox_state.sandboxes[i];
            
            if (!sb->enabled) {
                return 1;
            }
            
            if (size_kb > sb->max_file_size) {
                return 0;  /* Exceeds file size limit */
            }
            
            return 1;
        }
    }
    
    return 1;  /* No sandbox = allow */
}

/**
 * sandbox_set_max_open_files()
 * Set max open file descriptors
 */
int sandbox_set_max_open_files(uint32_t process_id, uint32_t max)
{
    for (uint32_t i = 0; i < sandbox_state.sandbox_count; i++) {
        if (sandbox_state.sandboxes[i].process_id == process_id) {
            sandbox_state.sandboxes[i].max_open_files = max;
            return 0;
        }
    }
    
    return -1;
}

/**
 * sandbox_set_device_access()
 * Configure device access mask
 */
int sandbox_set_device_access(uint32_t process_id, uint32_t device_mask)
{
    for (uint32_t i = 0; i < sandbox_state.sandbox_count; i++) {
        if (sandbox_state.sandboxes[i].process_id == process_id) {
            sandbox_state.sandboxes[i].allowed_devices = device_mask;
            return 0;
        }
    }
    
    return -1;
}

/**
 * sandbox_get_stats()
 * Get sandbox statistics
 */
void sandbox_get_stats(uint32_t *total_sandboxes, uint32_t *escape_attempts,
                       uint32_t *policy_violations)
{
    uint32_t enabled_count = 0;
    
    for (uint32_t i = 0; i < sandbox_state.sandbox_count; i++) {
        if (sandbox_state.sandboxes[i].enabled) {
            enabled_count++;
        }
    }
    
    if (total_sandboxes) *total_sandboxes = enabled_count;
    if (escape_attempts) *escape_attempts = sandbox_state.escape_attempts;
    if (policy_violations) *policy_violations = sandbox_state.policy_violations;
}

/**
 * sandbox_is_active()
 * Check if sandbox is active for process
 */
int sandbox_is_active(uint32_t process_id)
{
    for (uint32_t i = 0; i < sandbox_state.sandbox_count; i++) {
        if (sandbox_state.sandboxes[i].process_id == process_id &&
            sandbox_state.sandboxes[i].enabled) {
            return 1;
        }
    }
    
    return 0;
}

/**
 * sandbox_reset_stats()
 * Reset violation counters
 */
void sandbox_reset_stats(void)
{
    sandbox_state.escape_attempts = 0;
    sandbox_state.policy_violations = 0;
}
