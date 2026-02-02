#include "seleaf.h"
#include "../../include/serial.h"
#include <stddef.h>

/**
 * kernel/SELeaf/seleaf.c
 * 
 * SELeaf Security Module Implementation
 * Mandatory Access Control (MAC) for LeafOS kernel
 */

static seleaf_mode_t current_mode = SELEAF_MODE_PERMISSIVE;
static seleaf_policy_rule_t policy_rules[SELEAF_MAX_POLICIES];
static uint32_t policy_count = 0;
static seleaf_audit_entry_t audit_logs[SELEAF_MAX_AUDIT_LOGS];
static uint32_t audit_count = 0;

/**
 * seleaf_init()
 * Initialize SELeaf security module
 */
int seleaf_init(int mode)
{
    current_mode = (seleaf_mode_t)mode;
    policy_count = 0;
    audit_count = 0;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[    ] SELeaf: Initializing security module\n");
    #endif
    
    // Load default policies
    seleaf_load_policy();
    
    return 0;
}

/**
 * seleaf_set_mode()
 * Change SELeaf security mode (disabled/permissive/enforcing)
 */
int seleaf_set_mode(seleaf_mode_t mode)
{
    const char *mode_str;
    
    if (mode > SELEAF_MODE_ENFORCING) {
        return -1;
    }
    
    current_mode = mode;
    
    switch (mode) {
        case SELEAF_MODE_DISABLED:
            mode_str = "DISABLED";
            break;
        case SELEAF_MODE_PERMISSIVE:
            mode_str = "PERMISSIVE";
            break;
        case SELEAF_MODE_ENFORCING:
            mode_str = "ENFORCING";
            break;
        default:
            mode_str = "UNKNOWN";
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[    ] SELeaf: Mode set to ");
    serial_puts(SERIAL_PORT_A, (char *)mode_str);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return 0;
}

/**
 * seleaf_get_mode()
 * Get current SELeaf mode
 */
seleaf_mode_t seleaf_get_mode(void)
{
    return current_mode;
}

/**
 * seleaf_load_policy()
 * Load default security policies
 */
int seleaf_load_policy(void)
{
    // Initialize default policies
    // Allow kernel full access to everything
    if (policy_count < SELEAF_MAX_POLICIES) {
        policy_rules[policy_count].source_type = 1;  // kernel type
        policy_rules[policy_count].target_type = 0;  // any target
        policy_rules[policy_count].object_class = SELEAF_CLASS_FILE;
        policy_rules[policy_count].allowed_permissions = 0xFFFFFFFF;
        policy_rules[policy_count].denied_permissions = 0;
        policy_rules[policy_count].is_audit = 0;
        policy_count++;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[    ] SELeaf: ");
    serial_puts(SERIAL_PORT_A, "Loaded default policies\n");
    #endif
    
    return policy_count;
}

/**
 * seleaf_check_permission()
 * Check if subject has permission to perform action on object
 */
int seleaf_check_permission(seleaf_context_t *subject, seleaf_context_t *object,
                            seleaf_class_t object_class, uint32_t permission)
{
    if (current_mode == SELEAF_MODE_DISABLED) {
        return 1;  // Allow all
    }
    
    // Check policies
    for (uint32_t i = 0; i < policy_count; i++) {
        if (policy_rules[i].source_type == subject->type &&
            (policy_rules[i].target_type == 0 || policy_rules[i].target_type == object->type) &&
            policy_rules[i].object_class == object_class) {
            
            uint32_t allowed = policy_rules[i].allowed_permissions & permission;
            
            if (allowed == permission) {
                return 1;  // Permission granted
            }
            
            // Log denial in permissive mode
            if (current_mode == SELEAF_MODE_PERMISSIVE) {
                return 1;
            }
            
            return 0;  // Permission denied
        }
    }
    
    // Default deny
    if (current_mode == SELEAF_MODE_PERMISSIVE) {
        return 1;  // Allow in permissive mode
    }
    
    return 0;  // Deny in enforcing mode
}

/**
 * seleaf_log_audit()
 * Log security audit event
 */
int seleaf_log_audit(seleaf_audit_entry_t *entry)
{
    if (audit_count >= SELEAF_MAX_AUDIT_LOGS) {
        return -1;
    }
    
    audit_logs[audit_count] = *entry;
    audit_count++;
    
    return audit_count;
}

/**
 * seleaf_class_to_string()
 * Convert object class to string representation
 */
const char *seleaf_class_to_string(seleaf_class_t cls)
{
    switch (cls) {
        case SELEAF_CLASS_FILE:
            return "file";
        case SELEAF_CLASS_DIR:
            return "dir";
        case SELEAF_CLASS_PROCESS:
            return "process";
        case SELEAF_CLASS_SOCKET:
            return "socket";
        case SELEAF_CLASS_DEVICE:
            return "device";
        case SELEAF_CLASS_DRIVER:
            return "driver";
        case SELEAF_CLASS_MEMORY:
            return "memory";
        case SELEAF_CLASS_KERNEL:
            return "kernel";
        default:
            return "unknown";
    }
}

/**
 * seleaf_permission_to_string()
 * Convert permission to string representation
 */
const char *seleaf_permission_to_string(seleaf_permission_t perm)
{
    switch (perm) {
        case SELEAF_PERM_READ:
            return "read";
        case SELEAF_PERM_WRITE:
            return "write";
        case SELEAF_PERM_EXECUTE:
            return "execute";
        case SELEAF_PERM_CREATE:
            return "create";
        case SELEAF_PERM_DELETE:
            return "delete";
        case SELEAF_PERM_MOUNT:
            return "mount";
        case SELEAF_PERM_IOCTL:
            return "ioctl";
        default:
            return "unknown";
    }
}
/**
 * ENCRYPTION SUBSYSTEM
 * Provides data and kernel encryption capabilities
 */

static seleaf_encryption_key_t current_key = {0};
static seleaf_encryption_t current_algorithm = SELEAF_ENC_NONE;
static uint8_t kernel_protection_enabled = 0;

/**
 * seleaf_encrypt_init()
 * Initialize encryption subsystem
 */
int seleaf_encrypt_init(seleaf_encryption_t algorithm)
{
    current_algorithm = algorithm;
    kernel_protection_enabled = 0;
    
    #ifdef CONFIG_SERIAL_DRIVER
    switch (algorithm) {
        case SELEAF_ENC_NONE:
            serial_puts(SERIAL_PORT_A, "[    ] SELeaf Encryption: disabled\n");
            break;
        case SELEAF_ENC_XOR:
            serial_puts(SERIAL_PORT_A, "[    ] SELeaf Encryption: XOR enabled\n");
            break;
        case SELEAF_ENC_ROT13:
            serial_puts(SERIAL_PORT_A, "[    ] SELeaf Encryption: ROT13 enabled\n");
            break;
        case SELEAF_ENC_CUSTOM:
            serial_puts(SERIAL_PORT_A, "[    ] SELeaf Encryption: Custom cipher enabled\n");
            break;
    }
    #endif
    
    return 0;
}

/**
 * seleaf_set_encryption_key()
 * Set encryption key from user buffer
 */
int seleaf_set_encryption_key(uint8_t *key, uint32_t key_length)
{
    uint32_t copy_len;
    
    if (!key || key_length == 0) {
        return -1;
    }
    
    copy_len = key_length < SELEAF_ENCRYPTION_KEY_SIZE ? 
              key_length : SELEAF_ENCRYPTION_KEY_SIZE;
    
    for (uint32_t i = 0; i < copy_len; i++) {
        current_key.key[i] = key[i];
    }
    
    current_key.key_id = (uint32_t)(key[0] << 24 | key[1] << 16 | key[2] << 8 | key[3]);
    current_key.active = 1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[    ] SELeaf: Encryption key set (");
    serial_puts(SERIAL_PORT_A, "ID: 0x");
    for (int i = 0; i < 4; i++) {
        uint8_t nibble = (current_key.key_id >> (24 - i*8)) & 0xF;
        serial_putchar(SERIAL_PORT_A, (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10));
    }
    serial_puts(SERIAL_PORT_A, ")\n");
    #endif
    
    return 0;
}

/**
 * seleaf_encrypt_data()
 * Encrypt plaintext to ciphertext using current algorithm
 */
int seleaf_encrypt_data(uint8_t *plaintext, uint32_t length, uint8_t *ciphertext)
{
    if (!plaintext || !ciphertext || length == 0) {
        return -1;
    }
    
    if (current_algorithm == SELEAF_ENC_NONE) {
        // No encryption - just copy
        for (uint32_t i = 0; i < length; i++) {
            ciphertext[i] = plaintext[i];
        }
        return length;
    }
    
    if (current_algorithm == SELEAF_ENC_XOR) {
        // Simple XOR encryption with key
        for (uint32_t i = 0; i < length; i++) {
            uint8_t key_byte = current_key.key[i % SELEAF_ENCRYPTION_KEY_SIZE];
            ciphertext[i] = plaintext[i] ^ key_byte;
        }
        return length;
    }
    
    if (current_algorithm == SELEAF_ENC_ROT13) {
        // ROT13 cipher
        for (uint32_t i = 0; i < length; i++) {
            uint8_t c = plaintext[i];
            if (c >= 'a' && c <= 'z') {
                ciphertext[i] = ((c - 'a' + 13) % 26) + 'a';
            } else if (c >= 'A' && c <= 'Z') {
                ciphertext[i] = ((c - 'A' + 13) % 26) + 'A';
            } else if (c >= '0' && c <= '9') {
                ciphertext[i] = ((c - '0' + 5) % 10) + '0';
            } else {
                ciphertext[i] = c;
            }
        }
        return length;
    }
    
    // Custom cipher: complex XOR with key rotation
    for (uint32_t i = 0; i < length; i++) {
        uint8_t key_byte = current_key.key[(i * 7) % SELEAF_ENCRYPTION_KEY_SIZE];
        uint8_t rotation = ((i + 1) % 8);
        uint8_t rotated_key = (key_byte << rotation) | (key_byte >> (8 - rotation));
        ciphertext[i] = plaintext[i] ^ rotated_key;
    }
    return length;
}

/**
 * seleaf_decrypt_data()
 * Decrypt ciphertext to plaintext (XOR and custom are symmetric)
 */
int seleaf_decrypt_data(uint8_t *ciphertext, uint32_t length, uint8_t *plaintext)
{
    if (!ciphertext || !plaintext || length == 0) {
        return -1;
    }
    
    if (current_algorithm == SELEAF_ENC_NONE || current_algorithm == SELEAF_ENC_XOR ||
        current_algorithm == SELEAF_ENC_CUSTOM) {
        // XOR and custom are symmetric - use same operation
        return seleaf_encrypt_data(ciphertext, length, plaintext);
    }
    
    if (current_algorithm == SELEAF_ENC_ROT13) {
        // ROT13 is symmetric - apply again
        return seleaf_encrypt_data(ciphertext, length, plaintext);
    }
    
    return -1;
}

/**
 * seleaf_kernel_protect_enable()
 * Enable kernel memory protection and encryption
 */
int seleaf_kernel_protect_enable(void)
{
    kernel_protection_enabled = 1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[    ] SELeaf: Kernel protection ENABLED\n");
    #endif
    
    return 0;
}

/**
 * seleaf_kernel_protect_disable()
 * Disable kernel memory protection
 */
int seleaf_kernel_protect_disable(void)
{
    kernel_protection_enabled = 0;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[    ] SELeaf: Kernel protection DISABLED\n");
    #endif
    
    return 0;
}

/**
 * seleaf_get_encryption_status()
 * Get current encryption status
 */
int seleaf_get_encryption_status(void)
{
    int status = 0;
    
    if (current_algorithm != SELEAF_ENC_NONE) {
        status |= 0x01;  // Encryption enabled
    }
    
    if (kernel_protection_enabled) {
        status |= 0x02;  // Kernel protection enabled
    }
    
    if (current_key.active) {
        status |= 0x04;  // Encryption key set
    }
    
    return status;
}