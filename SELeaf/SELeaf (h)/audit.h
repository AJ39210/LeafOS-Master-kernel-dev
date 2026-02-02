/**
 * include/SELeaf/audit.h
 * 
 * SELeaf Audit and Logging System
 */

#ifndef SELEAF_AUDIT_H
#define SELEAF_AUDIT_H

#include <stdint.h>

/* Audit event types */
typedef enum {
    AUDIT_POLICY_LOADED = 0,
    AUDIT_POLICY_ENFORCED,
    AUDIT_CAPABILITY_DENIED,
    AUDIT_SYSCALL_DENIED,
    AUDIT_FILE_ACCESS,
    AUDIT_PROCESS_CREATED,
    AUDIT_PROCESS_KILLED,
    AUDIT_LABEL_CHANGED,
    AUDIT_SANDBOX_VIOLATION,
    AUDIT_LEARNING_MODE
} audit_event_type_t;

/* Audit event */
typedef struct {
    uint32_t timestamp;
    audit_event_type_t type;
    uint32_t pid;
    uint32_t data[4];
} audit_event_t;

/* Verbosity levels */
typedef enum {
    AUDIT_LEVEL_DISABLED = 0,
    AUDIT_LEVEL_ERRORS,
    AUDIT_LEVEL_ALL,
    AUDIT_LEVEL_VERBOSE
} audit_level_t;

/* Function declarations */
int audit_init(void);
int audit_log_event(audit_event_type_t type, uint32_t pid,
                   uint32_t data0, uint32_t data1,
                   uint32_t data2, uint32_t data3);
int audit_set_level(audit_level_t level);
int audit_enable(void);
int audit_disable(void);
int audit_get_level(void);
void audit_dump_recent(uint32_t count);
void audit_clear_log(void);
void audit_get_stats(uint32_t *total_events, uint32_t *buffered_events);
const char *audit_event_name(audit_event_type_t type);

#endif /* SELEAF_AUDIT_H */
