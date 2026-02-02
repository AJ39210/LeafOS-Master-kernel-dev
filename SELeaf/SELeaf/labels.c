/**
 * kernel/SELeaf/labels.c
 * 
 * SELeaf Label System
 * 
 * Provides:
 * - Process labels
 * - File labels
 * - Label-based access control
 * - Sandbox labels
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Label types */
typedef enum {
    LABEL_KERNEL,
    LABEL_TRUSTED,
    LABEL_UNTRUSTED,
    LABEL_SANDBOXED,
    LABEL_NETWORK,
    LABEL_IO_DEVICE,
    LABEL_SYSTEM_CALL,
    LABEL_CUSTOM
} label_type_t;

/* Label structure */
typedef struct {
    uint32_t id;
    label_type_t type;
    char name[32];
    uint32_t permissions;    /* Bitmask of allowed operations */
} label_t;

/* Process label mapping */
typedef struct {
    uint32_t process_id;
    uint32_t label_id;
} process_label_t;

/* File label mapping */
typedef struct {
    uint32_t inode;
    uint32_t label_id;
} file_label_t;

/* Label management state */
#define LABEL_MAX               64
#define PROCESS_LABEL_MAX       128
#define FILE_LABEL_MAX          256

typedef struct {
    label_t labels[LABEL_MAX];
    uint32_t label_count;
    
    process_label_t proc_labels[PROCESS_LABEL_MAX];
    uint32_t proc_label_count;
    
    file_label_t file_labels[FILE_LABEL_MAX];
    uint32_t file_label_count;
} label_state_t;

static label_state_t label_state = {0};

/**
 * label_init()
 * Initialize label system
 */
int label_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[LABELS] Label system initialized\n");
    #endif
    
    label_state.label_count = 0;
    label_state.proc_label_count = 0;
    label_state.file_label_count = 0;
    
    return 0;
}

/**
 * label_create()
 * Create a new label
 */
uint32_t label_create(label_type_t type, const char *name, uint32_t permissions)
{
    if (label_state.label_count >= LABEL_MAX) {
        return 0;  /* Error: table full */
    }
    
    label_t *label = &label_state.labels[label_state.label_count];
    label->id = label_state.label_count + 1;
    label->type = type;
    label->permissions = permissions;
    
    /* Copy name */
    int i = 0;
    while (name[i] && i < 31) {
        label->name[i] = name[i];
        i++;
    }
    label->name[i] = '\0';
    
    label_state.label_count++;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[LABELS] Created label: ");
    serial_puts(SERIAL_PORT_A, name);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return label->id;
}

/**
 * label_assign_process()
 * Assign label to process
 */
int label_assign_process(uint32_t process_id, uint32_t label_id)
{
    if (label_state.proc_label_count >= PROCESS_LABEL_MAX) {
        return -1;
    }
    
    /* Check if process already has label */
    for (uint32_t i = 0; i < label_state.proc_label_count; i++) {
        if (label_state.proc_labels[i].process_id == process_id) {
            label_state.proc_labels[i].label_id = label_id;  /* Update */
            return 0;
        }
    }
    
    /* Add new mapping */
    label_state.proc_labels[label_state.proc_label_count].process_id = process_id;
    label_state.proc_labels[label_state.proc_label_count].label_id = label_id;
    label_state.proc_label_count++;
    
    return 0;
}

/**
 * label_assign_file()
 * Assign label to file
 */
int label_assign_file(uint32_t inode, uint32_t label_id)
{
    if (label_state.file_label_count >= FILE_LABEL_MAX) {
        return -1;
    }
    
    /* Check if file already has label */
    for (uint32_t i = 0; i < label_state.file_label_count; i++) {
        if (label_state.file_labels[i].inode == inode) {
            label_state.file_labels[i].label_id = label_id;  /* Update */
            return 0;
        }
    }
    
    /* Add new mapping */
    label_state.file_labels[label_state.file_label_count].inode = inode;
    label_state.file_labels[label_state.file_label_count].label_id = label_id;
    label_state.file_label_count++;
    
    return 0;
}

/**
 * label_get_process()
 * Get label for process
 */
uint32_t label_get_process(uint32_t process_id)
{
    for (uint32_t i = 0; i < label_state.proc_label_count; i++) {
        if (label_state.proc_labels[i].process_id == process_id) {
            return label_state.proc_labels[i].label_id;
        }
    }
    
    return 0;  /* No label */
}

/**
 * label_get_file()
 * Get label for file
 */
uint32_t label_get_file(uint32_t inode)
{
    for (uint32_t i = 0; i < label_state.file_label_count; i++) {
        if (label_state.file_labels[i].inode == inode) {
            return label_state.file_labels[i].label_id;
        }
    }
    
    return 0;  /* No label */
}

/**
 * label_check_access()
 * Check if process with label can access resource with label
 * 
 * Returns: 1 if allowed, 0 if denied
 */
int label_check_access(uint32_t from_label, uint32_t to_label, uint32_t operation)
{
    /* Kernel label can access anything */
    if (from_label == 1) {  /* Assuming ID 1 is kernel */
        return 1;
    }
    
    if (to_label == 0) {
        return 1;  /* No label = unrestricted access */
    }
    
    /* Find target label and check permissions */
    for (uint32_t i = 0; i < label_state.label_count; i++) {
        if (label_state.labels[i].id == to_label) {
            label_t *label = &label_state.labels[i];
            
            /* Check if operation is allowed */
            if (label->permissions & (1 << operation)) {
                return 1;
            }
            break;
        }
    }
    
    return 0;  /* Default deny */
}

/**
 * label_type_name()
 * Get human-readable label type name
 */
const char *label_type_name(label_type_t type)
{
    switch (type) {
        case LABEL_KERNEL:      return "KERNEL";
        case LABEL_TRUSTED:     return "TRUSTED";
        case LABEL_UNTRUSTED:   return "UNTRUSTED";
        case LABEL_SANDBOXED:   return "SANDBOXED";
        case LABEL_NETWORK:     return "NETWORK";
        case LABEL_IO_DEVICE:   return "IO_DEVICE";
        case LABEL_SYSTEM_CALL: return "SYSTEM_CALL";
        case LABEL_CUSTOM:      return "CUSTOM";
        default:                return "UNKNOWN";
    }
}

/**
 * label_get_stats()
 * Get label statistics
 */
void label_get_stats(uint32_t *total_labels, uint32_t *proc_labels, uint32_t *file_labels)
{
    if (total_labels) *total_labels = label_state.label_count;
    if (proc_labels) *proc_labels = label_state.proc_label_count;
    if (file_labels) *file_labels = label_state.file_label_count;
}
