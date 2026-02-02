/**
 * include/SELeaf/syscall_filter.h
 * 
 * SELeaf Syscall Filtering (Seccomp-like)
 */

#ifndef SELEAF_SYSCALL_FILTER_H
#define SELEAF_SYSCALL_FILTER_H

#include <stdint.h>

/* Filter actions */
typedef enum {
    SYSCALL_FILTER_ALLOW = 0,
    SYSCALL_FILTER_DENY,
    SYSCALL_FILTER_KILL,
    SYSCALL_FILTER_TRACE
} syscall_filter_action_t;

/* Filter rule */
typedef struct {
    uint32_t syscall_num;
    syscall_filter_action_t action;
    uint32_t arg_mask;
    uint32_t arg_values[6];
} syscall_filter_rule_t;

/* Filter profile */
typedef struct {
    uint32_t id;
    char name[32];
    syscall_filter_rule_t rules[64];
    uint32_t rule_count;
} syscall_filter_profile_t;

/* Function declarations */
int syscall_filter_init(void);
uint32_t syscall_filter_create_profile(const char *name);
int syscall_filter_add_rule(uint32_t profile_id, uint32_t syscall_num,
                           syscall_filter_action_t action, uint32_t arg_mask);
int syscall_filter_check(uint32_t profile_id, uint32_t syscall_num,
                        uint32_t *args);
int syscall_filter_activate_profile(uint32_t profile_id);
int syscall_filter_deactivate_profile(uint32_t profile_id);
void syscall_filter_get_stats(uint32_t *violations, uint32_t *allowed);
const char *filter_action_name(syscall_filter_action_t action);

#endif /* SELEAF_SYSCALL_FILTER_H */
