/**
 * include/SELeaf/forensics.h
 */

#ifndef SELEAF_FORENSICS_H
#define SELEAF_FORENSICS_H

#include <stdint.h>

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

int forensics_init(void);
void forensics_enable(void);
void forensics_disable(void);
int forensics_log_event(forensic_event_type_t type, uint32_t pid, uint32_t uid,
                       uint32_t d0, uint32_t d1, uint32_t d2, uint32_t d3,
                       const char *resource);
int forensics_process_lifecycle(int is_create, uint32_t pid, uint32_t ppid, uint32_t uid);
int forensics_file_access(forensic_event_type_t access_type,
                         uint32_t pid, uint32_t uid,
                         const char *file_path, uint32_t size);
int forensics_network_event(int is_connect, uint32_t pid, uint32_t uid,
                           uint32_t remote_ip, uint16_t remote_port);
int forensics_detect_anomaly(uint32_t process_id);
void forensics_reconstruct_timeline(uint32_t process_id, uint32_t count);
void forensics_get_stats(uint32_t *total_events, uint32_t *suspicious, uint32_t *buffered);
void forensics_clear(void);

#endif
