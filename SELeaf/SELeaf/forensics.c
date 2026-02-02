/**
 * kernel/SELeaf/forensics.c
 * 
 * SELeaf Forensics Mode
 * 
 * Provides:
 * - Process lifecycle tracking
 * - File access auditing
 * - Network connection logging
 * - Memory access recording
 * - Forensic reconstruction
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Forensic event types */
typedef enum {
    FORENSIC_PROCESS_CREATE = 0,
    FORENSIC_PROCESS_EXIT,
    FORENSIC_FILE_OPEN,
    FORENSIC_FILE_CLOSE,
    FORENSIC_FILE_READ,
    FORENSIC_FILE_WRITE,
    FORENSIC_FILE_DELETE,
    FORENSIC_NETWORK_CONNECT,
    FORENSIC_NETWORK_LISTEN,
    FORENSIC_MEMORY_WRITE,
    FORENSIC_SYSCALL_INVOKE,
    FORENSIC_PRIVILEGE_ESCALATION,
    FORENSIC_SIGNAL_RECEIVED,
    FORENSIC_IPC_MESSAGE
} forensic_event_type_t;

/* Forensic event record */
typedef struct {
    uint32_t timestamp;
    forensic_event_type_t type;
    uint32_t process_id;
    uint32_t user_id;
    uint32_t data[4];           /* Type-specific data */
    char resource[64];          /* File path, network address, etc. */
} forensic_event_t;

/* Forensics state */
#define FORENSIC_EVENTS_MAX     4096

typedef struct {
    forensic_event_t events[FORENSIC_EVENTS_MAX];
    uint32_t event_count;
    uint32_t event_index;       /* Circular buffer index */
    
    int forensics_enabled;
    uint32_t total_events_logged;
    uint32_t suspicious_events;
} forensics_state_t;

static forensics_state_t forensics_state = {0};

/**
 * forensics_init()
 * Initialize forensics system
 */
int forensics_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[FORENSICS] Forensics mode initialized\n");
    #endif
    
    forensics_state.event_count = 0;
    forensics_state.event_index = 0;
    forensics_state.forensics_enabled = 1;
    forensics_state.total_events_logged = 0;
    forensics_state.suspicious_events = 0;
    
    return 0;
}

/**
 * forensics_enable()
 * Enable forensics recording
 */
void forensics_enable(void)
{
    forensics_state.forensics_enabled = 1;
}

/**
 * forensics_disable()
 * Disable forensics recording
 */
void forensics_disable(void)
{
    forensics_state.forensics_enabled = 0;
}

/**
 * forensics_log_event()
 * Log forensic event to circular buffer
 */
int forensics_log_event(forensic_event_type_t type, uint32_t pid, uint32_t uid,
                       uint32_t d0, uint32_t d1, uint32_t d2, uint32_t d3,
                       const char *resource)
{
    if (!forensics_state.forensics_enabled) {
        return -1;
    }
    
    forensic_event_t *event = &forensics_state.events[forensics_state.event_index];
    
    event->timestamp = 0;  /* TODO: get actual timestamp */
    event->type = type;
    event->process_id = pid;
    event->user_id = uid;
    event->data[0] = d0;
    event->data[1] = d1;
    event->data[2] = d2;
    event->data[3] = d3;
    
    /* Copy resource string */
    int i = 0;
    if (resource) {
        while (resource[i] && i < 63) {
            event->resource[i] = resource[i];
            i++;
        }
    }
    event->resource[i] = '\0';
    
    /* Move to next circular buffer position */
    forensics_state.event_index = (forensics_state.event_index + 1) % FORENSIC_EVENTS_MAX;
    
    if (forensics_state.event_count < FORENSIC_EVENTS_MAX) {
        forensics_state.event_count++;
    }
    
    forensics_state.total_events_logged++;
    
    /* Detect suspicious patterns */
    if (type == FORENSIC_PRIVILEGE_ESCALATION || 
        type == FORENSIC_FILE_DELETE ||
        type == FORENSIC_SYSCALL_INVOKE) {
        forensics_state.suspicious_events++;
    }
    
    return 0;
}

/**
 * forensics_process_lifecycle()
 * Log process creation and exit
 */
int forensics_process_lifecycle(int is_create, uint32_t pid, uint32_t ppid, uint32_t uid)
{
    forensic_event_type_t type = is_create ? 
        FORENSIC_PROCESS_CREATE : FORENSIC_PROCESS_EXIT;
    
    return forensics_log_event(type, pid, uid, ppid, 0, 0, 0, "");
}

/**
 * forensics_file_access()
 * Log file access event
 */
int forensics_file_access(forensic_event_type_t access_type, 
                         uint32_t pid, uint32_t uid, 
                         const char *file_path, uint32_t size)
{
    return forensics_log_event(access_type, pid, uid, size, 0, 0, 0, file_path);
}

/**
 * forensics_network_event()
 * Log network connection event
 */
int forensics_network_event(int is_connect, uint32_t pid, uint32_t uid,
                           uint32_t remote_ip, uint16_t remote_port)
{
    forensic_event_type_t type = is_connect ? 
        FORENSIC_NETWORK_CONNECT : FORENSIC_NETWORK_LISTEN;
    
    return forensics_log_event(type, pid, uid, remote_ip, remote_port, 0, 0, "");
}

/**
 * forensics_detect_anomaly()
 * Detect suspicious activity patterns
 * 
 * Returns: suspicious level (0=normal, 1=warning, 2=critical)
 */
int forensics_detect_anomaly(uint32_t process_id)
{
    uint32_t file_deletes = 0;
    uint32_t priv_escals = 0;
    uint32_t network_connects = 0;
    
    /* Scan recent events for process */
    uint32_t start = (forensics_state.event_index - 100) % FORENSIC_EVENTS_MAX;
    
    for (uint32_t i = 0; i < 100 && i < forensics_state.event_count; i++) {
        uint32_t idx = (start + i) % FORENSIC_EVENTS_MAX;
        forensic_event_t *e = &forensics_state.events[idx];
        
        if (e->process_id != process_id) continue;
        
        switch (e->type) {
            case FORENSIC_FILE_DELETE:
                file_deletes++;
                break;
            case FORENSIC_PRIVILEGE_ESCALATION:
                priv_escals++;
                break;
            case FORENSIC_NETWORK_CONNECT:
                network_connects++;
                break;
            default:
                break;
        }
    }
    
    /* Simple anomaly scoring */
    int score = 0;
    if (file_deletes > 10) score += 1;      /* Many deletions = suspicious */
    if (priv_escals > 0) score += 2;        /* Privilege escalation = critical */
    if (network_connects > 5) score += 1;   /* Many connections = suspicious */
    
    return (score > 2) ? 2 : (score > 0) ? 1 : 0;
}

/**
 * forensics_reconstruct_timeline()
 * Display timeline of events for process
 */
void forensics_reconstruct_timeline(uint32_t process_id, uint32_t count)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "\n[FORENSICS] Timeline for PID ");
    
    char pid_str[16];
    uint32_t temp = process_id;
    int len = 0;
    if (temp == 0) {
        pid_str[len++] = '0';
    } else {
        while (temp > 0) {
            pid_str[15 - len] = '0' + (temp % 10);
            temp /= 10;
            len++;
        }
    }
    if (len > 0) {
        serial_puts(SERIAL_PORT_A, &pid_str[15 - len + 1]);
    }
    
    serial_puts(SERIAL_PORT_A, "\n");
    serial_puts(SERIAL_PORT_A, "========================\n");
    
    uint32_t printed = 0;
    for (uint32_t i = 0; i < forensics_state.event_count && printed < count; i++) {
        uint32_t idx = (forensics_state.event_index + i) % FORENSIC_EVENTS_MAX;
        forensic_event_t *e = &forensics_state.events[idx];
        
        if (e->process_id != process_id) continue;
        
        const char *event_name = "UNKNOWN";
        switch (e->type) {
            case FORENSIC_PROCESS_CREATE: event_name = "CREATE"; break;
            case FORENSIC_PROCESS_EXIT: event_name = "EXIT"; break;
            case FORENSIC_FILE_OPEN: event_name = "OPEN"; break;
            case FORENSIC_FILE_READ: event_name = "READ"; break;
            case FORENSIC_FILE_WRITE: event_name = "WRITE"; break;
            case FORENSIC_FILE_DELETE: event_name = "DELETE"; break;
            case FORENSIC_NETWORK_CONNECT: event_name = "CONNECT"; break;
            case FORENSIC_PRIVILEGE_ESCALATION: event_name = "ESCALATE"; break;
            default: event_name = "OTHER";
        }
        
        serial_puts(SERIAL_PORT_A, event_name);
        
        if (e->resource[0]) {
            serial_puts(SERIAL_PORT_A, ": ");
            serial_puts(SERIAL_PORT_A, e->resource);
        }
        
        serial_puts(SERIAL_PORT_A, "\n");
        printed++;
    }
    #endif
}

/**
 * forensics_get_stats()
 * Get forensics statistics
 */
void forensics_get_stats(uint32_t *total_events, uint32_t *suspicious, 
                        uint32_t *buffered)
{
    if (total_events) *total_events = forensics_state.total_events_logged;
    if (suspicious) *suspicious = forensics_state.suspicious_events;
    if (buffered) *buffered = forensics_state.event_count;
}

/**
 * forensics_clear()
 * Clear forensic event buffer
 */
void forensics_clear(void)
{
    forensics_state.event_count = 0;
    forensics_state.event_index = 0;
}
