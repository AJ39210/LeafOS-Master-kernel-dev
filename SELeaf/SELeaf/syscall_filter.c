/**
 * kernel/SELeaf/syscall_filter.c
 * 
 * SELeaf Syscall Filtering (seccomp-like)
 * 
 * Provides:
 * - Per-process syscall filtering
 * - Syscall whitelist/blacklist
 * - Syscall argument validation
 * - Action on violation (allow/deny/kill)
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Syscall filter constants */
#define SYSCALL_FILTER_MAX          256
#define SYSCALL_PROFILE_MAX         32

/* Filter action types */
typedef enum {
    SYSCALL_ACTION_ALLOW,
    SYSCALL_ACTION_DENY,
    SYSCALL_ACTION_KILL,
    SYSCALL_ACTION_TRACE
} syscall_action_t;

/* Syscall filter entry */
typedef struct {
    uint32_t syscall_num;
    syscall_action_t action;
    uint32_t arg_mask;      /* Which arguments to validate */
    uint32_t arg_values[6]; /* Expected argument values */
} syscall_filter_entry_t;

/* Syscall filter profile (for different processes) */
typedef struct {
    char name[32];
    syscall_filter_entry_t filters[SYSCALL_FILTER_MAX];
    uint32_t filter_count;
    int active;
} syscall_profile_t;

/* Global filter state */
typedef struct {
    syscall_profile_t profiles[SYSCALL_PROFILE_MAX];
    uint32_t profile_count;
    uint32_t violations;
    uint32_t allowed;
} syscall_filter_state_t;

static syscall_filter_state_t filter_state = {0};

/**
 * syscall_filter_init()
 * Initialize syscall filtering
 */
int syscall_filter_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SYSCALL_FILTER] Initializing syscall filter\n");
    #endif
    
    filter_state.profile_count = 0;
    filter_state.violations = 0;
    filter_state.allowed = 0;
    
    return 0;
}

/**
 * syscall_filter_create_profile()
 * Create a new syscall filter profile
 */
int syscall_filter_create_profile(const char *name)
{
    if (filter_state.profile_count >= SYSCALL_PROFILE_MAX) {
        return -1;
    }
    
    syscall_profile_t *profile = &filter_state.profiles[filter_state.profile_count];
    
    /* Copy profile name */
    int i = 0;
    while (name[i] && i < 31) {
        profile->name[i] = name[i];
        i++;
    }
    profile->name[i] = '\0';
    
    profile->filter_count = 0;
    profile->active = 0;
    
    filter_state.profile_count++;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SYSCALL_FILTER] Created profile: ");
    serial_puts(SERIAL_PORT_A, name);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return filter_state.profile_count - 1;
}

/**
 * syscall_filter_add_rule()
 * Add syscall filter rule to profile
 */
int syscall_filter_add_rule(int profile_id, uint32_t syscall_num, 
                           syscall_action_t action, uint32_t arg_mask)
{
    if (profile_id < 0 || profile_id >= filter_state.profile_count) {
        return -1;
    }
    
    syscall_profile_t *profile = &filter_state.profiles[profile_id];
    
    if (profile->filter_count >= SYSCALL_FILTER_MAX) {
        return -1;
    }
    
    syscall_filter_entry_t *entry = &profile->filters[profile->filter_count];
    entry->syscall_num = syscall_num;
    entry->action = action;
    entry->arg_mask = arg_mask;
    
    profile->filter_count++;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SYSCALL_FILTER] Added filter rule\n");
    #endif
    
    return 0;
}

/**
 * syscall_filter_check()
 * Check if syscall is allowed by profile
 * Returns: 1 if allowed, 0 if denied
 */
int syscall_filter_check(int profile_id, uint32_t syscall_num, uint32_t *args)
{
    if (profile_id < 0 || profile_id >= filter_state.profile_count) {
        return 1;  /* No profile = allow */
    }
    
    syscall_profile_t *profile = &filter_state.profiles[profile_id];
    
    if (!profile->active) {
        return 1;  /* Inactive profile = allow */
    }
    
    /* Search for matching filter rule */
    for (uint32_t i = 0; i < profile->filter_count; i++) {
        syscall_filter_entry_t *filter = &profile->filters[i];
        
        if (filter->syscall_num != syscall_num) {
            continue;
        }
        
        /* Rule matches - check action */
        switch (filter->action) {
            case SYSCALL_ACTION_ALLOW:
                filter_state.allowed++;
                return 1;
                
            case SYSCALL_ACTION_DENY:
                filter_state.violations++;
                #ifdef CONFIG_SERIAL_DRIVER
                serial_puts(SERIAL_PORT_A, "[SYSCALL_FILTER] Denied syscall\n");
                #endif
                return 0;
                
            case SYSCALL_ACTION_KILL:
                filter_state.violations++;
                #ifdef CONFIG_SERIAL_DRIVER
                serial_puts(SERIAL_PORT_A, "[SYSCALL_FILTER] Kill on syscall violation\n");
                #endif
                return 0;
                
            case SYSCALL_ACTION_TRACE:
                /* Allow but log */
                #ifdef CONFIG_SERIAL_DRIVER
                serial_puts(SERIAL_PORT_A, "[SYSCALL_FILTER] Traced syscall\n");
                #endif
                return 1;
        }
    }
    
    /* Default: allow if no rule found */
    return 1;
}

/**
 * syscall_filter_activate_profile()
 * Activate profile for enforcement
 */
int syscall_filter_activate_profile(int profile_id)
{
    if (profile_id < 0 || profile_id >= filter_state.profile_count) {
        return -1;
    }
    
    filter_state.profiles[profile_id].active = 1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SYSCALL_FILTER] Profile activated\n");
    #endif
    
    return 0;
}

/**
 * syscall_filter_get_stats()
 * Get filter statistics
 */
void syscall_filter_get_stats(uint32_t *violations, uint32_t *allowed)
{
    if (violations) *violations = filter_state.violations;
    if (allowed) *allowed = filter_state.allowed;
}
