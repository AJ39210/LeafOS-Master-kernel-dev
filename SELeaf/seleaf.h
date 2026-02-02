#ifndef SELEAF_H
#define SELEAF_H

#include <stdint.h>

/**
 * kernel/SELeaf/seleaf.h
 * 
 * SELeaf - Security Module for LeafOS
 * Similar to SELinux but designed specifically for LeafOS
 * 
 * Provides:
 * - Mandatory Access Control (MAC)
 * - Security contexts and labels
 * - Policy enforcement
 * - Audit logging
 */

#define SELEAF_VERSION 0x010000  // v1.0.0
#define SELEAF_MAX_CONTEXTS 256
#define SELEAF_MAX_POLICIES 512
#define SELEAF_MAX_AUDIT_LOGS 1024
#define SELEAF_ENCRYPTION_KEY_SIZE 32  // 256-bit keys
#define SELEAF_MAX_ENCRYPTED_BUFFERS 64

// Encryption algorithms
typedef enum {
    SELEAF_ENC_NONE = 0,
    SELEAF_ENC_XOR = 1,        // Simple XOR encryption
    SELEAF_ENC_ROT13 = 2,      // ROT13 rotation
    SELEAF_ENC_CUSTOM = 3      // Custom kernel cipher
} seleaf_encryption_t;

// Encryption key structure
typedef struct {
    uint8_t key[SELEAF_ENCRYPTION_KEY_SIZE];
    uint32_t key_id;
    seleaf_encryption_t algorithm;
    uint8_t active;
} seleaf_encryption_key_t;

// SELinux-like security modes
typedef enum {
    SELEAF_MODE_DISABLED = 0,
    SELEAF_MODE_PERMISSIVE = 1,  // Log violations but allow access
    SELEAF_MODE_ENFORCING = 2    // Enforce policy strictly
} seleaf_mode_t;

// Object classes (similar to SELinux)
typedef enum {
    SELEAF_CLASS_FILE = 1,
    SELEAF_CLASS_DIR = 2,
    SELEAF_CLASS_PROCESS = 3,
    SELEAF_CLASS_SOCKET = 4,
    SELEAF_CLASS_DEVICE = 5,
    SELEAF_CLASS_DRIVER = 6,
    SELEAF_CLASS_MEMORY = 7,
    SELEAF_CLASS_KERNEL = 8
} seleaf_class_t;

// Permission types
typedef enum {
    SELEAF_PERM_READ = 0x01,
    SELEAF_PERM_WRITE = 0x02,
    SELEAF_PERM_EXECUTE = 0x04,
    SELEAF_PERM_CREATE = 0x08,
    SELEAF_PERM_DELETE = 0x10,
    SELEAF_PERM_MOUNT = 0x20,
    SELEAF_PERM_IOCTL = 0x40
} seleaf_permission_t;

// Security context structure
typedef struct {
    uint32_t id;
    uint8_t user;
    uint8_t role;
    uint16_t type;
    uint8_t level;
} seleaf_context_t;

// Security label structure
typedef struct {
    seleaf_context_t subject;
    seleaf_context_t object;
    seleaf_class_t object_class;
    uint32_t permissions;
} seleaf_label_t;

// Policy rule structure
typedef struct {
    uint16_t source_type;
    uint16_t target_type;
    seleaf_class_t object_class;
    uint32_t allowed_permissions;
    uint32_t denied_permissions;
    uint8_t is_audit;
} seleaf_policy_rule_t;

// Audit log entry
typedef struct {
    uint32_t timestamp;
    seleaf_context_t subject;
    seleaf_context_t object;
    seleaf_class_t object_class;
    uint32_t requested_permission;
    uint8_t denied;
    uint8_t in_enforcing_mode;
} seleaf_audit_entry_t;

// Functions
int seleaf_init(int mode);
int seleaf_set_mode(seleaf_mode_t mode);
seleaf_mode_t seleaf_get_mode(void);
int seleaf_load_policy(void);
int seleaf_check_permission(seleaf_context_t *subject, seleaf_context_t *object, 
                            seleaf_class_t object_class, uint32_t permission);
int seleaf_log_audit(seleaf_audit_entry_t *entry);
const char *seleaf_class_to_string(seleaf_class_t cls);
const char *seleaf_permission_to_string(seleaf_permission_t perm);

// Encryption functions
int seleaf_encrypt_init(seleaf_encryption_t algorithm);
int seleaf_encrypt_data(uint8_t *plaintext, uint32_t length, uint8_t *ciphertext);
int seleaf_decrypt_data(uint8_t *ciphertext, uint32_t length, uint8_t *plaintext);
int seleaf_set_encryption_key(uint8_t *key, uint32_t key_length);
int seleaf_kernel_protect_enable(void);
int seleaf_kernel_protect_disable(void);
int seleaf_get_encryption_status(void);

#endif
