/**
 * kernel/SELeaf/audit.c
 * 
 * SELeaf Audit & Logging System
 * 
 * Provides:
 * - Security event logging
 * - Audit trail
 * - Event filtering
 * - Statistics
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Audit event types */
typedef enum {
    AUDIT_POLICY_LOADED,
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
    audit_event_type_t type;
    uint32_t process_id;
    uint32_t timestamp;
    uint32_t data[4];        /* Event-specific data */
} audit_event_t;

/* Audit configuration */
#define AUDIT_LOG_MAX           1024
#define AUDIT_BUFFER_SIZE       32

typedef struct {
    audit_event_t events[AUDIT_LOG_MAX];
    uint32_t event_count;
    uint32_t event_index;      /* Circular buffer index */
    uint32_t total_events;     /* Total events since boot */
    int enabled;
    int level;                 /* 0=none, 1=errors, 2=all, 3=verbose */
} audit_state_t;

static audit_state_t audit_state = {0};

/**
 * audit_init()
 * Initialize audit system
 */
int audit_init(void)
{
    audit_state.enabled = 1;
    audit_state.level = 2;  /* Log all by default */
    audit_state.event_count = 0;
    audit_state.event_index = 0;
    audit_state.total_events = 0;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[AUDIT] Audit system initialized (level=2)\n");
    #endif
    
    return 0;
}

/**
 * audit_log_event()
 * Log a security event
 */
int audit_log_event(audit_event_type_t type, uint32_t process_id, 
                    uint32_t data0, uint32_t data1, uint32_t data2, uint32_t data3)
{
    if (!audit_state.enabled) {
        return -1;
    }
    
    /* Use circular buffer */
    uint32_t idx = audit_state.event_index;
    audit_event_t *event = &audit_state.events[idx];
    
    event->type = type;
    event->process_id = process_id;
    event->timestamp = 0;  /* Would use real timestamp */
    event->data[0] = data0;
    event->data[1] = data1;
    event->data[2] = data2;
    event->data[3] = data3;
    
    /* Advance circular buffer */
    audit_state.event_index = (idx + 1) % AUDIT_LOG_MAX;
    if (audit_state.event_count < AUDIT_LOG_MAX) {
        audit_state.event_count++;
    }
    audit_state.total_events++;
    
    /* Log to serial if enabled */
    if (audit_state.level >= 2) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[AUDIT] Event: ");
        audit_print_event_type(type);
        serial_puts(SERIAL_PORT_A, " (PID=");
        // Print PID
        serial_puts(SERIAL_PORT_A, ")\n");
        #endif
    }
    
    return 0;
}

/**
 * audit_print_event_type()
 * Print human-readable event type
 */
void audit_print_event_type(audit_event_type_t type)
{
    #ifdef CONFIG_SERIAL_DRIVER
    switch (type) {
        case AUDIT_POLICY_LOADED:
            serial_puts(SERIAL_PORT_A, "POLICY_LOADED");
            break;
        case AUDIT_POLICY_ENFORCED:
            serial_puts(SERIAL_PORT_A, "POLICY_ENFORCED");
            break;
        case AUDIT_CAPABILITY_DENIED:
            serial_puts(SERIAL_PORT_A, "CAPABILITY_DENIED");
            break;
        case AUDIT_SYSCALL_DENIED:
            serial_puts(SERIAL_PORT_A, "SYSCALL_DENIED");
            break;
        case AUDIT_FILE_ACCESS:
            serial_puts(SERIAL_PORT_A, "FILE_ACCESS");
            break;
        case AUDIT_PROCESS_CREATED:
            serial_puts(SERIAL_PORT_A, "PROCESS_CREATED");
            break;
        case AUDIT_PROCESS_KILLED:
            serial_puts(SERIAL_PORT_A, "PROCESS_KILLED");
            break;
        case AUDIT_LABEL_CHANGED:
            serial_puts(SERIAL_PORT_A, "LABEL_CHANGED");
            break;
        case AUDIT_SANDBOX_VIOLATION:
            serial_puts(SERIAL_PORT_A, "SANDBOX_VIOLATION");
            break;
        case AUDIT_LEARNING_MODE:
            serial_puts(SERIAL_PORT_A, "LEARNING_MODE");
            break;
        default:
            serial_puts(SERIAL_PORT_A, "UNKNOWN");
    }
    #endif
}

/**
 * audit_set_level()
 * Set audit logging level
 * 0=disabled, 1=errors only, 2=all, 3=verbose
 */
void audit_set_level(int level)
{
    audit_state.level = level;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[AUDIT] Level set to ");
    serial_putchar(SERIAL_PORT_A, '0' + level);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
}

/**
 * audit_enable()
 * Enable audit logging
 */
void audit_enable(void)
{
    audit_state.enabled = 1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[AUDIT] Audit logging ENABLED\n");
    #endif
}

/**
 * audit_disable()
 * Disable audit logging
 */
void audit_disable(void)
{
    audit_state.enabled = 0;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[AUDIT] Audit logging DISABLED\n");
    #endif
}

/**
 * audit_get_stats()
 * Get audit statistics
 */
void audit_get_stats(uint32_t *total_events, uint32_t *buffered_events)
{
    if (total_events) *total_events = audit_state.total_events;
    if (buffered_events) *buffered_events = audit_state.event_count;
}

/**
 * audit_dump_recent()
 * Dump recent audit events to serial
 */
void audit_dump_recent(uint32_t count)
{
    if (count > AUDIT_BUFFER_SIZE) {
        count = AUDIT_BUFFER_SIZE;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "\n[AUDIT] Recent ");
    serial_putchar(SERIAL_PORT_A, '0' + count);
    serial_puts(SERIAL_PORT_A, " events:\n");
    
    uint32_t start = (audit_state.event_index - count + AUDIT_LOG_MAX) % AUDIT_LOG_MAX;
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t idx = (start + i) % AUDIT_LOG_MAX;
        audit_event_t *event = &audit_state.events[idx];
        
        serial_puts(SERIAL_PORT_A, "  [");
        audit_print_event_type(event->type);
        serial_puts(SERIAL_PORT_A, "] PID=");
        // Print event details
        serial_puts(SERIAL_PORT_A, "\n");
    }
    #endif
}

/**
 * audit_clear_log()
 * Clear audit log buffer
 */
void audit_clear_log(void)
{
    audit_state.event_count = 0;
    audit_state.event_index = 0;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[AUDIT] Log cleared\n");
    #endif
}
