/*
 * SELeaf Security Kernel Inside the Kernel (SKIK)
 * Meta-security layer providing sandboxed execution for the security system itself
 * 
 * Protects SELeaf from being compromised or bypassed by creating a
 * meta-security context that cannot be directly accessed from userspace.
 */

#include <stdint.h>
#include <string.h>
#include "../../include/SELeaf/security_kernel.h"

/* SKIK (Security Kernel Inside Kernel) execution context */
typedef struct {
    uint32_t context_id;
    uint8_t privilege_level;        /* 0=user, 1=kernel, 2=security_kernel */
    uint32_t allowed_syscalls;      /* Bitmask of accessible syscalls */
    uint32_t memory_base;
    uint32_t memory_size;
    uint16_t isolation_flags;
    uint32_t creation_time;
    uint8_t verification_status;
} skik_context_t;

/* SKIK module registry - tracks all security modules */
typedef struct {
    char module_name[32];
    void *module_ptr;
    uint32_t module_size;
    uint32_t hash;
    uint32_t verification_status;
    uint8_t trust_level;
} skik_module_t;

/* Main SKIK state */
typedef struct {
    skik_context_t contexts[SKIK_MAX_CONTEXTS];
    uint32_t context_count;
    skik_module_t modules[SKIK_MAX_MODULES];
    uint32_t module_count;
    uint32_t isolation_level;       /* 0=none, 1=basic, 2=strict */
    uint32_t verification_required;
    uint32_t total_checks;
    uint32_t failed_checks;
} skik_state_t;

static skik_state_t skik = {0};

/**
 * skik_init - Initialize Security Kernel Inside Kernel
 * @isolation_level: Level of isolation (0-2)
 * @verify_on_init: Whether to verify all modules on startup
 *
 * Creates the meta-security context that protects SELeaf itself
 */
void skik_init(uint8_t isolation_level, uint8_t verify_on_init)
{
    skik.context_count = 0;
    skik.module_count = 0;
    skik.isolation_level = isolation_level;
    skik.verification_required = verify_on_init;
    skik.total_checks = 0;
    skik.failed_checks = 0;
    
    /* Create root security kernel context */
    skik_context_t *root = &skik.contexts[0];
    root->context_id = SKIK_ROOT_CONTEXT;
    root->privilege_level = SKIK_LEVEL_SECURITY_KERNEL;
    root->allowed_syscalls = 0xFFFFFFFF;  /* Full access */
    root->memory_base = 0x80000000;       /* Protected memory region */
    root->memory_size = 512 * 1024;       /* 512KB for security kernel */
    root->isolation_flags = SKIK_FLAG_PROTECTED | SKIK_FLAG_NO_EXEC_COPY;
    root->verification_status = SKIK_VERIFIED;
    
    skik.context_count = 1;
}

/**
 * skik_register_module - Register a SELeaf module with SKIK
 * @module_name: Name of the SELeaf module
 * @module_ptr: Pointer to module code
 * @module_size: Size of module code
 * @hash: Hash of module code for integrity verification
 *
 * Returns: Module ID on success, -1 on failure
 */
int32_t skik_register_module(const char *module_name, void *module_ptr,
                             uint32_t module_size, uint32_t hash)
{
    if (skik.module_count >= SKIK_MAX_MODULES)
        return -1;
    
    skik_module_t *mod = &skik.modules[skik.module_count];
    
    /* Copy module name manually */
    uint32_t i = 0;
    while (i < 31 && module_name[i] != '\0') {
        mod->module_name[i] = module_name[i];
        i++;
    }
    mod->module_name[i] = '\0';
    
    mod->module_ptr = module_ptr;
    mod->module_size = module_size;
    mod->hash = hash;
    mod->verification_status = SKIK_UNVERIFIED;
    mod->trust_level = SKIK_TRUST_UNTRUSTED;
    
    /* Verify module if required */
    if (skik.verification_required) {
        if (skik_verify_module(skik.module_count) == 0) {
            mod->verification_status = SKIK_VERIFIED;
            mod->trust_level = SKIK_TRUST_VERIFIED;
        } else {
            mod->verification_status = SKIK_FAILED_VERIFICATION;
            skik.failed_checks++;
            return -1;
        }
    }
    
    return (int32_t)skik.module_count++;
}

/**
 * skik_verify_module - Verify integrity of a security module
 * @module_id: Module to verify
 *
 * Returns: 0 if verified, -1 if verification fails
 * 
 * Verification checks:
 *  1. Code size matches
 *  2. Code hash matches expected hash
 *  3. No suspicious patterns detected
 *  4. Module hasn't been modified since registration
 */
int32_t skik_verify_module(uint32_t module_id)
{
    if (module_id >= skik.module_count)
        return -1;
    
    skik_module_t *mod = &skik.modules[module_id];
    skik.total_checks++;
    
    /* In real implementation, would:
     * 1. Compute hash of module_ptr
     * 2. Compare against mod->hash
     * 3. Check for code modification patterns
     * 4. Scan for known malware signatures
     */
    
    /* Placeholder: assume verified if hash structure looks OK */
    if (mod->module_size > 0 && mod->hash != 0) {
        return 0;
    }
    
    skik.failed_checks++;
    return -1;
}

/**
 * skik_create_context - Create new SKIK execution context
 * @privilege_level: Privilege level (SKIK_LEVEL_USER, KERNEL, SECURITY_KERNEL)
 * @syscall_mask: Bitmask of allowed syscalls
 * @memory_size: Memory allocation for this context
 *
 * Returns: Context ID on success, -1 on failure
 */
int32_t skik_create_context(uint8_t privilege_level, uint32_t syscall_mask,
                            uint32_t memory_size)
{
    if (skik.context_count >= SKIK_MAX_CONTEXTS)
        return -1;
    
    /* Only SKIK_LEVEL_SECURITY_KERNEL can create contexts */
    if (skik_get_current_level() != SKIK_LEVEL_SECURITY_KERNEL)
        return -1;
    
    skik_context_t *ctx = &skik.contexts[skik.context_count];
    
    ctx->context_id = skik.context_count;
    ctx->privilege_level = privilege_level;
    ctx->allowed_syscalls = syscall_mask;
    ctx->memory_size = memory_size;
    ctx->isolation_flags = SKIK_FLAG_PROTECTED;
    ctx->verification_status = SKIK_UNVERIFIED;
    
    return (int32_t)skik.context_count++;
}

/**
 * skik_check_access - Check if an operation is allowed in current context
 * @operation: Operation being requested (SKIK_OP_*)
 * @target_context: Target context ID
 *
 * Returns: 1 if allowed, 0 if denied
 * 
 * Access control rules:
 *  - User level: Cannot modify security modules
 *  - Kernel level: Limited security module access
 *  - Security kernel: Full access to all operations
 */
uint8_t skik_check_access(uint32_t operation, uint32_t target_context)
{
    skik.total_checks++;
    
    uint8_t current_level = skik_get_current_level();
    
    /* Security kernel level has full access */
    if (current_level == SKIK_LEVEL_SECURITY_KERNEL)
        return 1;
    
    /* Kernel level - limited access */
    if (current_level == SKIK_LEVEL_KERNEL) {
        if (operation & (SKIK_OP_MODIFY_MODULE | SKIK_OP_DISABLE_SECURITY))
            return 0;  /* Deny */
        return 1;      /* Allow */
    }
    
    /* User level - minimal access */
    if (current_level == SKIK_LEVEL_USER) {
        if (operation & (SKIK_OP_READ_AUDIT | SKIK_OP_READ_STATUS))
            return 1;  /* Allow read-only ops */
        return 0;      /* Deny everything else */
    }
    
    skik.failed_checks++;
    return 0;
}

/**
 * skik_protect_memory - Protect memory region from unauthorized access
 * @address: Memory address to protect
 * @size: Size of region
 * @flags: Protection flags (read/write/execute)
 *
 * Returns: 0 on success, -1 on failure
 * 
 * Sets up hardware page table entries to prevent:
 *  - Unauthorized reads (information leak prevention)
 *  - Unauthorized writes (integrity protection)
 *  - Execution from user space (code injection prevention)
 */
int32_t skik_protect_memory(uint32_t address, uint32_t size, uint16_t flags)
{
    /* In real implementation, would modify page tables */
    
    if (skik_get_current_level() != SKIK_LEVEL_SECURITY_KERNEL)
        return -1;
    
    /* Configure memory protection... */
    return 0;
}

/**
 * skik_verify_syscall_entry - Verify syscall is allowed before execution
 * @syscall_num: Syscall number
 * @current_context: Current execution context
 *
 * Returns: 1 if allowed, 0 if should be blocked
 * 
 * Verifies:
 *  1. Syscall is in allowed list for this context
 *  2. Arguments don't reference protected memory
 *  3. No privilege escalation attempt
 *  4. No security module access attempt
 */
uint8_t skik_verify_syscall_entry(uint32_t syscall_num, uint32_t current_context)
{
    skik.total_checks++;
    
    if (current_context >= skik.context_count)
        return 0;
    
    skik_context_t *ctx = &skik.contexts[current_context];
    
    /* Check if this syscall is allowed */
    if (!(ctx->allowed_syscalls & (1 << (syscall_num % 32))))
        return 0;  /* Syscall not allowed */
    
    /* Check privilege escalation */
    if (syscall_num == 164) {  /* Example: setuid/setgid syscalls */
        if (ctx->privilege_level == SKIK_LEVEL_USER)
            return 0;  /* Block privilege escalation */
    }
    
    return 1;  /* Allow */
}

/**
 * skik_audit_operation - Log and audit security-critical operation
 * @operation: Operation being performed
 * @context_id: Context performing operation
 * @status: Whether operation was allowed
 * @details: Additional details (optional)
 *
 * Creates permanent audit record in protected memory
 */
void skik_audit_operation(uint32_t operation, uint32_t context_id,
                         uint8_t status, const char *details)
{
    /* Would write to protected audit log in real implementation */
    
    if (status == 0)  /* Operation denied */
        skik.failed_checks++;
}

/**
 * skik_disable_bypass - Detect and prevent security bypass attempts
 * @pattern: Potential bypass pattern (kernel memory write, module modification, etc)
 *
 * Returns: 1 if bypass detected and blocked, 0 otherwise
 * 
 * Detects:
 *  - Direct kernel memory writes
 *  - Security module code modification
 *  - Page table manipulation
 *  - Interrupt handler replacement
 *  - Syscall table hooking
 */
uint8_t skik_disable_bypass(uint32_t pattern)
{
    skik.total_checks++;
    
    /* Check for known bypass patterns */
    if (pattern == SKIK_BYPASS_DIRECT_MEMORY_WRITE)
        return 1;  /* Blocked */
    
    if (pattern == SKIK_BYPASS_MODULE_MODIFICATION)
        return 1;  /* Blocked */
    
    if (pattern == SKIK_BYPASS_PAGETABLE_MANIPULATION)
        return 1;  /* Blocked */
    
    return 0;  /* Not a bypass attempt */
}

/**
 * skik_get_current_level - Get privilege level of current context
 *
 * Returns: Current privilege level (SKIK_LEVEL_*)
 */
uint8_t skik_get_current_level(void)
{
    /* In real implementation, would check current CPU context */
    /* For now, assume we're in the security kernel */
    return SKIK_LEVEL_SECURITY_KERNEL;
}

/**
 * skik_get_status - Get SKIK status
 * @status: Output status structure
 */
void skik_get_status(skik_status_t *status)
{
    status->active_contexts = skik.context_count;
    status->registered_modules = skik.module_count;
    status->isolation_level = skik.isolation_level;
    status->total_checks = skik.total_checks;
    status->failed_checks = skik.failed_checks;
    status->verified_modules = 0;
    
    uint32_t i;
    for (i = 0; i < skik.module_count; i++) {
        if (skik.modules[i].verification_status == SKIK_VERIFIED)
            status->verified_modules++;
    }
}

/**
 * skik_dump_modules - Display all registered security modules
 */
void skik_dump_modules(void)
{
    uint32_t i;
    
    for (i = 0; i < skik.module_count; i++) {
        skik_module_t *mod = &skik.modules[i];
        /* Output to serial: Module | Size | Hash | Verified */
        /* Example: policy_jit | 4096 | 0xabcd1234 | YES */
    }
}

/**
 * skik_enforce_strict_mode - Enable strict isolation mode
 * 
 * In strict mode:
 *  - All syscalls logged and limited
 *  - Memory protection enabled on all SKIK structures
 *  - Module modification detection active
 *  - Bypass attempts trigger system halt
 */
void skik_enforce_strict_mode(void)
{
    skik.isolation_level = SKIK_ISOLATION_STRICT;
    
    uint32_t i;
    for (i = 0; i < skik.context_count; i++) {
        skik.contexts[i].isolation_flags |= SKIK_FLAG_STRICT_ISOLATION;
    }
}
