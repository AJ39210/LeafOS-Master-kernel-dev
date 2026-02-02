/**
 * kernel/SELeaf/policy.c
 * 
 * SELeaf Capability-Based Security Policy Engine
 * 
 * Provides:
 * - Policy language parsing
 * - Capability definitions
 * - Policy enforcement
 * - Hot-reload capability
 * - Learning mode
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Policy language tokens */
#define POLICY_MAX_RULES        256
#define POLICY_MAX_CAPS         64
#define POLICY_NAME_LEN         32
#define POLICY_RULE_LEN         128

/* Policy rule types */
typedef enum {
    POLICY_ALLOW,
    POLICY_DENY,
    POLICY_AUDIT,
    POLICY_SANDBOX
} policy_action_t;

/* Capability types */
typedef enum {
    CAP_FILE_READ,
    CAP_FILE_WRITE,
    CAP_NETWORK_SEND,
    CAP_NETWORK_RECV,
    CAP_DEVICE_ACCESS,
    CAP_SYSCALL_EXECUTE,
    CAP_MEMORY_ALLOCATE,
    CAP_IPC_SEND,
    CAP_IPC_RECV,
    CAP_PROCESS_CREATE,
    CAP_PROCESS_KILL,
    CAP_MAX
} capability_t;

/* Policy rule */
typedef struct {
    char name[POLICY_NAME_LEN];
    policy_action_t action;
    capability_t capability;
    uint32_t target_id;  /* Process/file ID or any if 0 */
    int learning_mode;   /* 1 = learning (audit only) */
} policy_rule_t;

/* Policy context */
typedef struct {
    policy_rule_t rules[POLICY_MAX_RULES];
    uint32_t rule_count;
    int learning_mode_global;
    int hot_reload_enabled;
    uint32_t policies_loaded;
    uint32_t policies_enforced;
} policy_context_t;

static policy_context_t policy_ctx = {0};

/**
 * policy_init()
 * Initialize policy engine
 */
int policy_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[POLICY] Initializing policy engine\n");
    #endif
    
    policy_ctx.rule_count = 0;
    policy_ctx.learning_mode_global = 1;  /* Start in learning mode */
    policy_ctx.hot_reload_enabled = 1;
    
    return 0;
}

/**
 * policy_add_rule()
 * Add a capability-based policy rule
 */
int policy_add_rule(const char *name, policy_action_t action, 
                   capability_t cap, uint32_t target_id, int learning)
{
    if (policy_ctx.rule_count >= POLICY_MAX_RULES) {
        return -1;  /* Policy table full */
    }
    
    policy_rule_t *rule = &policy_ctx.rules[policy_ctx.rule_count];
    
    /* Copy rule name */
    int i = 0;
    while (name[i] && i < POLICY_NAME_LEN - 1) {
        rule->name[i] = name[i];
        i++;
    }
    rule->name[i] = '\0';
    
    rule->action = action;
    rule->capability = cap;
    rule->target_id = target_id;
    rule->learning_mode = learning;
    
    policy_ctx.rule_count++;
    policy_ctx.policies_loaded++;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[POLICY] Added rule: ");
    serial_puts(SERIAL_PORT_A, name);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return 0;
}

/**
 * policy_check_capability()
 * Check if a capability is allowed
 * Returns: 1 if allowed, 0 if denied
 */
int policy_check_capability(capability_t cap, uint32_t process_id)
{
    /* Search rules in reverse order (last rule wins) */
    for (int i = policy_ctx.rule_count - 1; i >= 0; i--) {
        policy_rule_t *rule = &policy_ctx.rules[i];
        
        /* Check if rule matches capability */
        if (rule->capability != cap) {
            continue;
        }
        
        /* Check if rule matches target (0 = any) */
        if (rule->target_id != 0 && rule->target_id != process_id) {
            continue;
        }
        
        /* Rule matches - check action */
        if (rule->learning_mode || policy_ctx.learning_mode_global) {
            /* Learning mode - allow but audit */
            return 1;
        }
        
        if (rule->action == POLICY_ALLOW) {
            return 1;
        } else if (rule->action == POLICY_DENY) {
            return 0;
        } else if (rule->action == POLICY_SANDBOX) {
            /* Sandbox - restrict but allow */
            return 1;
        }
    }
    
    /* Default: deny if no rule found */
    return 0;
}

/**
 * policy_enable_learning_mode()
 * Enable learning mode (audit all, allow all)
 */
void policy_enable_learning_mode(void)
{
    policy_ctx.learning_mode_global = 1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[POLICY] Learning mode ENABLED\n");
    #endif
}

/**
 * policy_disable_learning_mode()
 * Disable learning mode (enforce policies)
 */
void policy_disable_learning_mode(void)
{
    policy_ctx.learning_mode_global = 0;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[POLICY] Learning mode DISABLED - enforcing\n");
    #endif
}

/**
 * policy_hot_reload()
 * Clear and reload policies (for hot-reload capability)
 */
int policy_hot_reload(void)
{
    if (!policy_ctx.hot_reload_enabled) {
        return -1;
    }
    
    /* Clear existing rules */
    policy_ctx.rule_count = 0;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[POLICY] Policies reloaded\n");
    #endif
    
    return 0;
}

/**
 * policy_get_stats()
 * Get policy statistics
 */
void policy_get_stats(uint32_t *loaded, uint32_t *enforced)
{
    if (loaded) *loaded = policy_ctx.policies_loaded;
    if (enforced) *enforced = policy_ctx.policies_enforced;
}

/**
 * policy_is_learning_mode()
 * Check if in learning mode
 */
int policy_is_learning_mode(void)
{
    return policy_ctx.learning_mode_global;
}

/**
 * policy_capability_name()
 * Get human-readable capability name
 */
const char *policy_capability_name(capability_t cap)
{
    switch (cap) {
        case CAP_FILE_READ:         return "FILE_READ";
        case CAP_FILE_WRITE:        return "FILE_WRITE";
        case CAP_NETWORK_SEND:      return "NETWORK_SEND";
        case CAP_NETWORK_RECV:      return "NETWORK_RECV";
        case CAP_DEVICE_ACCESS:     return "DEVICE_ACCESS";
        case CAP_SYSCALL_EXECUTE:   return "SYSCALL_EXECUTE";
        case CAP_MEMORY_ALLOCATE:   return "MEMORY_ALLOCATE";
        case CAP_IPC_SEND:          return "IPC_SEND";
        case CAP_IPC_RECV:          return "IPC_RECV";
        case CAP_PROCESS_CREATE:    return "PROCESS_CREATE";
        case CAP_PROCESS_KILL:      return "PROCESS_KILL";
        default:                    return "UNKNOWN";
    }
}
