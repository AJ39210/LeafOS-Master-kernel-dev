/*
 * SELeaf Security Kernel Inside Kernel (SKIK) Header
 */

#ifndef SELEAF_SECURITY_KERNEL_H
#define SELEAF_SECURITY_KERNEL_H

#include <stdint.h>

/* Maximum number of SKIK contexts */
#define SKIK_MAX_CONTEXTS               16

/* Maximum number of modules under SKIK protection */
#define SKIK_MAX_MODULES                32

/* SKIK context IDs */
#define SKIK_ROOT_CONTEXT               0
#define SKIK_POLICY_CONTEXT             1
#define SKIK_AUDIT_CONTEXT              2

/* Privilege levels */
#define SKIK_LEVEL_USER                 0    /* User space */
#define SKIK_LEVEL_KERNEL               1    /* Kernel space */
#define SKIK_LEVEL_SECURITY_KERNEL      2    /* Meta-security layer */

/* Isolation levels */
#define SKIK_ISOLATION_NONE             0    /* No isolation */
#define SKIK_ISOLATION_BASIC            1    /* Basic isolation */
#define SKIK_ISOLATION_STRICT           2    /* Strict isolation */

/* Verification status */
#define SKIK_UNVERIFIED                 0
#define SKIK_VERIFIED                   1
#define SKIK_FAILED_VERIFICATION        2

/* Trust levels for modules */
#define SKIK_TRUST_UNTRUSTED            0
#define SKIK_TRUST_PROVISIONAL          1
#define SKIK_TRUST_VERIFIED             2
#define SKIK_TRUST_CRITICAL             3

/* Operations that require security checks */
#define SKIK_OP_READ_AUDIT              (1 << 0)
#define SKIK_OP_READ_STATUS             (1 << 1)
#define SKIK_OP_MODIFY_POLICY           (1 << 2)
#define SKIK_OP_MODIFY_MODULE           (1 << 3)
#define SKIK_OP_DISABLE_SECURITY        (1 << 4)
#define SKIK_OP_MEMORY_PROTECT          (1 << 5)
#define SKIK_OP_CREATE_CONTEXT          (1 << 6)
#define SKIK_OP_BYPASS_CHECK            (1 << 7)

/* Known bypass patterns */
#define SKIK_BYPASS_DIRECT_MEMORY_WRITE     1
#define SKIK_BYPASS_MODULE_MODIFICATION     2
#define SKIK_BYPASS_PAGETABLE_MANIPULATION  3
#define SKIK_BYPASS_INTERRUPT_HOOK          4
#define SKIK_BYPASS_SYSCALL_TABLE_HOOK      5

/* Context flags */
#define SKIK_FLAG_PROTECTED             (1 << 0)
#define SKIK_FLAG_NO_EXEC_COPY          (1 << 1)
#define SKIK_FLAG_STRICT_ISOLATION      (1 << 2)
#define SKIK_FLAG_AUDIT_ALL             (1 << 3)

/* Status structure */
typedef struct {
    uint32_t active_contexts;
    uint32_t registered_modules;
    uint8_t isolation_level;
    uint32_t total_checks;
    uint32_t failed_checks;
    uint32_t verified_modules;
} skik_status_t;

/* Function declarations */
void skik_init(uint8_t isolation_level, uint8_t verify_on_init);

int32_t skik_register_module(const char *module_name, void *module_ptr,
                             uint32_t module_size, uint32_t hash);

int32_t skik_verify_module(uint32_t module_id);

int32_t skik_create_context(uint8_t privilege_level, uint32_t syscall_mask,
                            uint32_t memory_size);

uint8_t skik_check_access(uint32_t operation, uint32_t target_context);

int32_t skik_protect_memory(uint32_t address, uint32_t size, uint16_t flags);

uint8_t skik_verify_syscall_entry(uint32_t syscall_num, uint32_t current_context);

void skik_audit_operation(uint32_t operation, uint32_t context_id,
                         uint8_t status, const char *details);

uint8_t skik_disable_bypass(uint32_t pattern);

uint8_t skik_get_current_level(void);

void skik_get_status(skik_status_t *status);

void skik_dump_modules(void);

void skik_enforce_strict_mode(void);

#endif /* SELEAF_SECURITY_KERNEL_H */
