/**
 * include/SELeaf/policy_generator.h
 */

#ifndef SELEAF_POLICY_GENERATOR_H
#define SELEAF_POLICY_GENERATOR_H

#include <stdint.h>

typedef enum {
    TEMPLATE_WEB_SERVER = 0,
    TEMPLATE_DATABASE,
    TEMPLATE_FILE_SYSTEM,
    TEMPLATE_NETWORK_UTILITY,
    TEMPLATE_SYSTEM_DAEMON,
    TEMPLATE_USER_APP,
    TEMPLATE_UNTRUSTED
} policy_template_t;

typedef struct {
    uint32_t policy_id;
    char name[32];
    uint32_t capabilities;
    uint32_t syscalls_allowed;
    policy_template_t template_type;
} generated_policy_t;

int policy_generator_init(void);
uint32_t policy_generate_from_template(policy_template_t template_type,
                                       const char *policy_name);
uint32_t policy_learn_from_behavior(const char *policy_name,
                                   uint32_t observed_capabilities,
                                   uint32_t observed_syscalls);
int policy_get_generated(uint32_t policy_id, generated_policy_t *out_policy);
void policy_generator_get_stats(uint32_t *total, uint32_t *generations);

#endif
