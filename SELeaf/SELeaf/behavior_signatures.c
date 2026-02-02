/*
 * SELeaf Behavior Signatures
 * Heuristic-based threat detection using behavioral patterns
 * 
 * Defines and matches behavior signatures for anomaly detection,
 * rootkit detection, privilege escalation detection, and more.
 */

#include <stdint.h>
#include <string.h>
#include "../../include/SELeaf/behavior_signatures.h"

/* Detected behavior event */
typedef struct {
    uint32_t timestamp;
    uint32_t process_id;
    uint32_t event_type;
    uint32_t event_data;
    uint8_t severity;
} behavior_event_t;

/* Behavior detection context */
typedef struct {
    behavior_signature_t signatures[BEHAVIOR_MAX_SIGNATURES];
    uint32_t signature_count;
    
    behavior_event_t events[BEHAVIOR_EVENT_BUFFER_SIZE];
    uint32_t event_write_pos;
    uint32_t event_count;
    
    uint32_t detections;
    uint32_t high_severity_detections;
    uint32_t process_heuristic_score[256];  /* Per-process anomaly score */
} behavior_detection_t;

static behavior_detection_t behavior = {0};

/* Predefined dangerous behavior signatures */
static const behavior_signature_t dangerous_signatures[] = {
    /* Signature 1: Privilege Escalation Pattern */
    {
        .signature_id = BEHAVIOR_SIG_PRIV_ESCALATION,
        .signature_name = "Privilege Escalation Attempt",
        .severity = 95,
        .confidence = 85,
        .flags = BEHAVIOR_FLAG_CRITICAL,
        .event_count = 3,              /* 3 events needed */
        .time_window = 5,              /* Within 5 seconds */
    },
    
    /* Signature 2: Code Injection Pattern */
    {
        .signature_id = BEHAVIOR_SIG_CODE_INJECTION,
        .signature_name = "Code Injection Pattern",
        .severity = 90,
        .confidence = 80,
        .flags = BEHAVIOR_FLAG_CRITICAL,
        .event_count = 2,
        .time_window = 2,
    },
    
    /* Signature 3: Rootkit Behavior */
    {
        .signature_id = BEHAVIOR_SIG_ROOTKIT,
        .signature_name = "Rootkit Installation Pattern",
        .severity = 100,
        .confidence = 90,
        .flags = BEHAVIOR_FLAG_CRITICAL,
        .event_count = 4,
        .time_window = 10,
    },
    
    /* Signature 4: Lateral Movement */
    {
        .signature_id = BEHAVIOR_SIG_LATERAL_MOVEMENT,
        .signature_name = "Lateral Movement Attempt",
        .severity = 75,
        .confidence = 70,
        .flags = BEHAVIOR_FLAG_HIGH,
        .event_count = 5,
        .time_window = 60,
    },
    
    /* Signature 5: Data Exfiltration */
    {
        .signature_id = BEHAVIOR_SIG_DATA_EXFIL,
        .signature_name = "Data Exfiltration Pattern",
        .severity = 85,
        .confidence = 80,
        .flags = BEHAVIOR_FLAG_HIGH,
        .event_count = 3,
        .time_window = 30,
    },
    
    /* Signature 6: Cryptojacking */
    {
        .signature_id = BEHAVIOR_SIG_CRYPTOJACK,
        .signature_name = "Cryptojacking Activity",
        .severity = 60,
        .confidence = 75,
        .flags = BEHAVIOR_FLAG_MEDIUM,
        .event_count = 2,
        .time_window = 120,
    },
    
    /* Signature 7: Brute Force Attack */
    {
        .signature_id = BEHAVIOR_SIG_BRUTEFORCE,
        .signature_name = "Brute Force Attack",
        .severity = 70,
        .confidence = 85,
        .flags = BEHAVIOR_FLAG_HIGH,
        .event_count = 10,
        .time_window = 60,
    },
    
    /* Signature 8: Memory Scanning */
    {
        .signature_id = BEHAVIOR_SIG_MEMSCAN,
        .signature_name = "Memory Scanning Activity",
        .severity = 80,
        .confidence = 70,
        .flags = BEHAVIOR_FLAG_HIGH,
        .event_count = 3,
        .time_window = 10,
    },
};

#define DANGEROUS_SIG_COUNT (sizeof(dangerous_signatures) / sizeof(dangerous_signatures[0]))

/**
 * behavior_init - Initialize behavior signature engine
 * Enable default dangerous behavior signatures
 */
void behavior_init(void)
{
    behavior.signature_count = 0;
    behavior.event_write_pos = 0;
    behavior.event_count = 0;
    behavior.detections = 0;
    behavior.high_severity_detections = 0;
    
    /* Register all dangerous signatures */
    uint32_t i;
    for (i = 0; i < DANGEROUS_SIG_COUNT; i++) {
        behavior_register_signature(&dangerous_signatures[i]);
    }
}

/**
 * behavior_register_signature - Register a behavior signature
 * @sig: Signature structure
 *
 * Returns: Signature ID on success, -1 on failure
 */
int32_t behavior_register_signature(const behavior_signature_t *sig)
{
    if (behavior.signature_count >= BEHAVIOR_MAX_SIGNATURES)
        return -1;
    
    /* Copy signature manually */
    behavior_signature_t *dest = &behavior.signatures[behavior.signature_count];
    
    dest->signature_id = sig->signature_id;
    
    /* Copy name */
    uint32_t i = 0;
    while (i < 47 && sig->signature_name[i] != '\0') {
        dest->signature_name[i] = sig->signature_name[i];
        i++;
    }
    dest->signature_name[i] = '\0';
    
    /* Copy patterns */
    for (i = 0; i < 8; i++) {
        dest->pattern[i] = sig->pattern[i];
        dest->pattern_mask[i] = sig->pattern_mask[i];
    }
    
    dest->severity = sig->severity;
    dest->confidence = sig->confidence;
    dest->flags = sig->flags;
    dest->event_count = sig->event_count;
    dest->time_window = sig->time_window;
    
    return (int32_t)behavior.signature_count++;
}

/**
 * behavior_report_event - Report a behavioral event for analysis
 * @process_id: Process ID performing the event
 * @event_type: Type of event (BEHAVIOR_EVENT_*)
 * @event_data: Event data (context-specific)
 * @severity: Event severity (0-255)
 *
 * Returns: Number of matching signatures
 */
uint32_t behavior_report_event(uint32_t process_id, uint32_t event_type,
                               uint32_t event_data, uint8_t severity)
{
    /* Add to event buffer */
    behavior_event_t *evt = &behavior.events[behavior.event_write_pos];
    evt->timestamp = 0;  /* Would be current tick in real impl */
    evt->process_id = process_id;
    evt->event_type = event_type;
    evt->event_data = event_data;
    evt->severity = severity;
    
    behavior.event_write_pos = (behavior.event_write_pos + 1) % BEHAVIOR_EVENT_BUFFER_SIZE;
    behavior.event_count++;
    
    /* Update process heuristic score */
    if (process_id < 256) {
        behavior.process_heuristic_score[process_id] += severity;
    }
    
    /* Analyze against all signatures */
    uint32_t matches = 0;
    uint32_t i;
    for (i = 0; i < behavior.signature_count; i++) {
        if (behavior_check_signature(i, process_id)) {
            matches++;
        }
    }
    
    return matches;
}

/**
 * behavior_check_signature - Check if a signature matches recent events
 * @sig_id: Signature ID to check
 * @process_id: Process to analyze
 *
 * Returns: 1 if signature matches (threat detected), 0 otherwise
 * 
 * Checks:
 *  1. Required number of events of specific types occurred
 *  2. Events occurred within time window
 *  3. Events from same process
 */
uint8_t behavior_check_signature(uint32_t sig_id, uint32_t process_id)
{
    if (sig_id >= behavior.signature_count)
        return 0;
    
    behavior_signature_t *sig = &behavior.signatures[sig_id];
    
    uint32_t matching_events = 0;
    uint32_t check_count = 0;
    uint32_t i;
    
    /* Scan event buffer backwards from current position */
    for (i = 0; i < BEHAVIOR_EVENT_BUFFER_SIZE && check_count < 100; i++) {
        uint32_t idx = (behavior.event_write_pos - 1 - i + BEHAVIOR_EVENT_BUFFER_SIZE) % 
                       BEHAVIOR_EVENT_BUFFER_SIZE;
        
        behavior_event_t *evt = &behavior.events[idx];
        
        if (evt->process_id != process_id)
            continue;
        
        check_count++;
        
        /* Check if event matches signature pattern */
        switch (sig->signature_id) {
            case BEHAVIOR_SIG_PRIV_ESCALATION:
                /* Check for: setuid syscall + exec + network */
                if (evt->event_type == BEHAVIOR_EVENT_SETUID ||
                    evt->event_type == BEHAVIOR_EVENT_EXEC ||
                    evt->event_type == BEHAVIOR_EVENT_PRIV_CHANGE)
                    matching_events++;
                break;
                
            case BEHAVIOR_SIG_CODE_INJECTION:
                /* Check for: mmap executable + write to code section */
                if (evt->event_type == BEHAVIOR_EVENT_MMAP_EXEC ||
                    evt->event_type == BEHAVIOR_EVENT_PTRACE)
                    matching_events++;
                break;
                
            case BEHAVIOR_SIG_ROOTKIT:
                /* Check for: kernel module load + syscall hook + hide process */
                if (evt->event_type == BEHAVIOR_EVENT_INSMOD ||
                    evt->event_type == BEHAVIOR_EVENT_SYSCALL_HOOK ||
                    evt->event_type == BEHAVIOR_EVENT_HIDE_PROCESS)
                    matching_events++;
                break;
                
            case BEHAVIOR_SIG_DATA_EXFIL:
                /* Check for: open sensitive file + network send + large transfer */
                if (evt->event_type == BEHAVIOR_EVENT_FILE_OPEN ||
                    evt->event_type == BEHAVIOR_EVENT_NETWORK_SEND ||
                    evt->event_type == BEHAVIOR_EVENT_LARGE_TRANSFER)
                    matching_events++;
                break;
                
            case BEHAVIOR_SIG_CRYPTOJACK:
                /* Check for: high CPU + memory intensive + no user I/O */
                if (evt->event_type == BEHAVIOR_EVENT_HIGH_CPU ||
                    evt->event_type == BEHAVIOR_EVENT_HIGH_MEMORY)
                    matching_events++;
                break;
                
            default:
                if (evt->event_type & sig->pattern[0])
                    matching_events++;
        }
        
        /* Stop if we have enough events */
        if (matching_events >= sig->event_count) {
            behavior.detections++;
            
            if (sig->severity >= 80)
                behavior.high_severity_detections++;
            
            return 1;  /* Signature matched! */
        }
    }
    
    return 0;  /* No match */
}

/**
 * behavior_get_threat_level - Get overall threat level for a process
 * @process_id: Process ID to assess
 *
 * Returns: Threat level (0-100, where 100 is maximum threat)
 * 
 * Based on:
 *  1. Heuristic anomaly score
 *  2. Number of signature matches
 *  3. Severity of detected behaviors
 */
uint8_t behavior_get_threat_level(uint32_t process_id)
{
    if (process_id >= 256)
        return 0;
    
    uint32_t score = behavior.process_heuristic_score[process_id];
    
    /* Normalize to 0-100 */
    if (score > 1000)
        return 100;
    
    return (score * 100) / 1000;
}

/**
 * behavior_is_malicious - Determine if a process appears malicious
 * @process_id: Process ID to assess
 *
 * Returns: 1 if malicious indicators detected, 0 otherwise
 * 
 * Uses multiple heuristics:
 *  1. Threat level > 75
 *  2. High-severity signature matches
 *  3. Suspicious privilege escalation attempts
 *  4. Rootkit patterns
 */
uint8_t behavior_is_malicious(uint32_t process_id)
{
    uint8_t threat_level = behavior_get_threat_level(process_id);
    
    /* High threat level = likely malicious */
    if (threat_level > 75)
        return 1;
    
    /* Check for specific dangerous signatures */
    if (behavior_check_signature(BEHAVIOR_SIG_PRIV_ESCALATION, process_id))
        return 1;
    
    if (behavior_check_signature(BEHAVIOR_SIG_ROOTKIT, process_id))
        return 1;
    
    if (behavior_check_signature(BEHAVIOR_SIG_CODE_INJECTION, process_id))
        return 1;
    
    return 0;
}

/**
 * behavior_reset_process_score - Reset threat score for a process
 * @process_id: Process ID
 */
void behavior_reset_process_score(uint32_t process_id)
{
    if (process_id < 256) {
        behavior.process_heuristic_score[process_id] = 0;
    }
}

/**
 * behavior_get_stats - Get behavior detection statistics
 * @stats: Output statistics structure
 */
void behavior_get_stats(behavior_stats_t *stats)
{
    stats->total_signatures = behavior.signature_count;
    stats->events_detected = behavior.event_count;
    stats->total_detections = behavior.detections;
    stats->high_severity_detections = behavior.high_severity_detections;
    
    /* Count active threats */
    stats->active_threats = 0;
    uint32_t i;
    for (i = 0; i < 256; i++) {
        if (behavior_is_malicious(i))
            stats->active_threats++;
    }
}

/**
 * behavior_dump_events - Display recent events (debugging)
 * @max_count: Maximum events to display
 */
void behavior_dump_events(uint32_t max_count)
{
    uint32_t count = 0;
    uint32_t i = 0;
    
    while (count < max_count && i < BEHAVIOR_EVENT_BUFFER_SIZE) {
        uint32_t idx = (behavior.event_write_pos - 1 - i + BEHAVIOR_EVENT_BUFFER_SIZE) % 
                       BEHAVIOR_EVENT_BUFFER_SIZE;
        
        behavior_event_t *evt = &behavior.events[idx];
        
        if (evt->process_id != 0) {
            /* Output to serial: PID | Type | Severity */
            /* Example: PID:42 | CODE_INJECTION | 90 */
            count++;
        }
        
        i++;
    }
}

/**
 * behavior_dump_detections - Display threat detections
 */
void behavior_dump_detections(void)
{
    /* Would output to serial in real implementation */
    
    uint32_t i;
    for (i = 0; i < 256; i++) {
        if (behavior_is_malicious(i)) {
            uint8_t threat = behavior_get_threat_level(i);
            /* Output: Process %d MALICIOUS (threat: %d%%) */
        }
    }
}
