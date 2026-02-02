/**
 * kernel/SELeaf/policy_generator.c
 * 
 * SELeaf Policy Generator
 * 
 * Provides:
 * - AI-style simple policy generation
 * - Baseline policy creation
 * - Learned policy synthesis
 * - Policy templates
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Policy template types */
typedef enum {
    TEMPLATE_WEB_SERVER = 0,
    TEMPLATE_DATABASE,
    TEMPLATE_FILE_SYSTEM,
    TEMPLATE_NETWORK_UTILITY,
    TEMPLATE_SYSTEM_DAEMON,
    TEMPLATE_USER_APP,
    TEMPLATE_UNTRUSTED
} policy_template_t;

/* Generated policy */
typedef struct {
    uint32_t policy_id;
    char name[32];
    uint32_t capabilities;
    uint32_t syscalls_allowed;
    policy_template_t template_type;
} generated_policy_t;

/* Policy generator state */
#define GENERATED_POLICIES_MAX  64

typedef struct {
    generated_policy_t policies[GENERATED_POLICIES_MAX];
    uint32_t policy_count;
    uint32_t generations;
} generator_state_t;

static generator_state_t gen_state = {0};

/**
 * policy_generator_init()
 */
int policy_generator_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[POLICY_GEN] Policy generator initialized\n");
    #endif
    gen_state.policy_count = 0;
    gen_state.generations = 0;
    return 0;
}

/**
 * policy_generate_from_template()
 */
uint32_t policy_generate_from_template(policy_template_t template_type,
                                       const char *policy_name)
{
    if (gen_state.policy_count >= GENERATED_POLICIES_MAX) return 0;
    
    generated_policy_t *pol = &gen_state.policies[gen_state.policy_count];
    pol->policy_id = gen_state.policy_count + 1;
    pol->template_type = template_type;
    
    int i = 0;
    while (policy_name[i] && i < 31) {
        pol->name[i] = policy_name[i];
        i++;
    }
    pol->name[i] = '\0';
    
    /* Generate capabilities based on template */
    switch (template_type) {
        case TEMPLATE_WEB_SERVER:
            pol->capabilities = 0x0F;     /* Read, write, network */
            pol->syscalls_allowed = 0x1FF;
            break;
        case TEMPLATE_DATABASE:
            pol->capabilities = 0x1F;     /* File + network + memory */
            pol->syscalls_allowed = 0x3FF;
            break;
        case TEMPLATE_FILE_SYSTEM:
            pol->capabilities = 0x07;     /* File operations only */
            pol->syscalls_allowed = 0x0FF;
            break;
        case TEMPLATE_NETWORK_UTILITY:
            pol->capabilities = 0x18;     /* Network only */
            pol->syscalls_allowed = 0x180;
            break;
        case TEMPLATE_SYSTEM_DAEMON:
            pol->capabilities = 0x3F;     /* Full system */
            pol->syscalls_allowed = 0x7FF;
            break;
        case TEMPLATE_USER_APP:
            pol->capabilities = 0x0F;     /* Limited */
            pol->syscalls_allowed = 0x1FF;
            break;
        case TEMPLATE_UNTRUSTED:
            pol->capabilities = 0x01;     /* Read only */
            pol->syscalls_allowed = 0x01F;
            break;
        default:
            pol->capabilities = 0x00;
            pol->syscalls_allowed = 0x00;
    }
    
    gen_state.policy_count++;
    gen_state.generations++;
    
    return pol->policy_id;
}

/**
 * policy_learn_from_behavior()
 * Generate policy from observed behavior
 */
uint32_t policy_learn_from_behavior(const char *policy_name,
                                   uint32_t observed_capabilities,
                                   uint32_t observed_syscalls)
{
    if (gen_state.policy_count >= GENERATED_POLICIES_MAX) return 0;
    
    generated_policy_t *pol = &gen_state.policies[gen_state.policy_count];
    pol->policy_id = gen_state.policy_count + 1;
    pol->template_type = TEMPLATE_USER_APP;
    
    int i = 0;
    while (policy_name[i] && i < 31) {
        pol->name[i] = policy_name[i];
        i++;
    }
    pol->name[i] = '\0';
    
    /* Use observed behavior as base */
    pol->capabilities = observed_capabilities;
    pol->syscalls_allowed = observed_syscalls;
    
    gen_state.policy_count++;
    gen_state.generations++;
    
    return pol->policy_id;
}

/**
 * policy_get_generated()
 */
int policy_get_generated(uint32_t policy_id, generated_policy_t *out_policy)
{
    if (policy_id == 0 || policy_id > gen_state.policy_count) return -1;
    
    *out_policy = gen_state.policies[policy_id - 1];
    return 0;
}

/**
 * policy_generator_get_stats()
 */
void policy_generator_get_stats(uint32_t *total, uint32_t *generations)
{
    if (total) *total = gen_state.policy_count;
    if (generations) *generations = gen_state.generations;
}
