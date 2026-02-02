/**
 * include/SELeaf/policy.h
 * 
 * SELeaf Capability-Based Policy Engine
 */

#ifndef SELEAF_POLICY_H
#define SELEAF_POLICY_H

#include <stdint.h>

/* Capability types */
typedef enum {
    CAP_FILE_READ = 0,
    CAP_FILE_WRITE,
    CAP_FILE_EXECUTE,
    CAP_NETWORK_SEND,
    CAP_NETWORK_RECV,
    CAP_DEVICE_ACCESS,
    CAP_SYSCALL_EXECUTE,
    CAP_MEMORY_ALLOCATE,
    CAP_IPC_SEND,
    CAP_IPC_RECV,
    CAP_PROCESS_CREATE,
    CAP_PROCESS_KILL
} capability_t;

/* Policy action */
typedef enum {
    POLICY_ALLOW = 0,
    POLICY_DENY,
    POLICY_AUDIT
} policy_action_t;

/* Policy rule */
typedef struct {
    uint32_t id;
    char name[32];
    policy_action_t action;
    capability_t capability;
    uint32_t target_id;
    int learning_mode;
} policy_rule_t;

/* Function declarations */
int policy_init(void);
uint32_t policy_add_rule(const char *name, policy_action_t action,
                        capability_t cap, uint32_t target_id, int learning);
int policy_check_capability(capability_t cap, uint32_t process_id);
int policy_enable_learning_mode(void);
int policy_disable_learning_mode(void);
int policy_hot_reload(void);
int policy_get_learning_mode(void);
void policy_get_stats(uint32_t *total_rules, uint32_t *violations);
const char *capability_name(capability_t cap);

#endif /* SELEAF_POLICY_H */
