/*
 * SELeaf Behavior Signatures Header
 */

#ifndef SELEAF_BEHAVIOR_SIGNATURES_H
#define SELEAF_BEHAVIOR_SIGNATURES_H

#include <stdint.h>

/* Maximum number of behavior signatures */
#define BEHAVIOR_MAX_SIGNATURES         32

/* Event buffer size (circular) */
#define BEHAVIOR_EVENT_BUFFER_SIZE      512

/* Behavior signature IDs */
#define BEHAVIOR_SIG_PRIV_ESCALATION    0
#define BEHAVIOR_SIG_CODE_INJECTION     1
#define BEHAVIOR_SIG_ROOTKIT            2
#define BEHAVIOR_SIG_LATERAL_MOVEMENT   3
#define BEHAVIOR_SIG_DATA_EXFIL         4
#define BEHAVIOR_SIG_CRYPTOJACK         5
#define BEHAVIOR_SIG_BRUTEFORCE         6
#define BEHAVIOR_SIG_MEMSCAN            7

/* Behavior event types */
#define BEHAVIOR_EVENT_SETUID           (1 << 0)
#define BEHAVIOR_EVENT_EXEC             (1 << 1)
#define BEHAVIOR_EVENT_PRIV_CHANGE      (1 << 2)
#define BEHAVIOR_EVENT_MMAP_EXEC        (1 << 3)
#define BEHAVIOR_EVENT_PTRACE           (1 << 4)
#define BEHAVIOR_EVENT_INSMOD           (1 << 5)
#define BEHAVIOR_EVENT_SYSCALL_HOOK     (1 << 6)
#define BEHAVIOR_EVENT_HIDE_PROCESS     (1 << 7)
#define BEHAVIOR_EVENT_FILE_OPEN        (1 << 8)
#define BEHAVIOR_EVENT_NETWORK_SEND     (1 << 9)
#define BEHAVIOR_EVENT_LARGE_TRANSFER   (1 << 10)
#define BEHAVIOR_EVENT_HIGH_CPU         (1 << 11)
#define BEHAVIOR_EVENT_HIGH_MEMORY      (1 << 12)

/* Signature severity levels */
#define BEHAVIOR_SEVERITY_LOW           20
#define BEHAVIOR_SEVERITY_MEDIUM        50
#define BEHAVIOR_SEVERITY_HIGH          75
#define BEHAVIOR_SEVERITY_CRITICAL      95

/* Signature flags */
#define BEHAVIOR_FLAG_CRITICAL          (1 << 0)
#define BEHAVIOR_FLAG_HIGH              (1 << 1)
#define BEHAVIOR_FLAG_MEDIUM            (1 << 2)
#define BEHAVIOR_FLAG_LOW               (1 << 3)
#define BEHAVIOR_FLAG_ANOMALY_DETECTION (1 << 4)
#define BEHAVIOR_FLAG_MACHINE_LEARNED   (1 << 5)

/* Behavior signature rule definition */
typedef struct {
    uint32_t signature_id;
    char signature_name[48];
    uint32_t pattern[8];            /* Pattern matching rules */
    uint32_t pattern_mask[8];       /* Pattern mask */
    uint8_t severity;               /* 0-255, higher = more severe */
    uint8_t confidence;             /* 0-100 */
    uint16_t flags;
    uint32_t event_count;           /* Events needed to trigger */
    uint32_t time_window;           /* Time window in seconds */
} behavior_signature_t;

/* Statistics structure */
typedef struct {
    uint32_t total_signatures;
    uint32_t events_detected;
    uint32_t total_detections;
    uint32_t high_severity_detections;
    uint32_t active_threats;
} behavior_stats_t;

/* Function declarations */
void behavior_init(void);

int32_t behavior_register_signature(const behavior_signature_t *sig);

uint32_t behavior_report_event(uint32_t process_id, uint32_t event_type,
                               uint32_t event_data, uint8_t severity);

uint8_t behavior_check_signature(uint32_t sig_id, uint32_t process_id);

uint8_t behavior_get_threat_level(uint32_t process_id);

uint8_t behavior_is_malicious(uint32_t process_id);

void behavior_reset_process_score(uint32_t process_id);

void behavior_get_stats(behavior_stats_t *stats);

void behavior_dump_events(uint32_t max_count);

void behavior_dump_detections(void);

#endif /* SELEAF_BEHAVIOR_SIGNATURES_H */
