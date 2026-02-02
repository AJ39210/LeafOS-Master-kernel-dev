/**
 * include/SELeaf/labels.h
 * 
 * SELeaf Label Management System
 */

#ifndef SELEAF_LABELS_H
#define SELEAF_LABELS_H

#include <stdint.h>

/* Label types */
typedef enum {
    LABEL_KERNEL = 0,
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
    uint32_t permissions;
} label_t;

/* Function declarations */
int label_init(void);
uint32_t label_create(label_type_t type, const char *name,
                     uint32_t permissions);
int label_assign_process(uint32_t process_id, uint32_t label_id);
int label_assign_file(uint32_t inode, uint32_t label_id);
uint32_t label_get_process(uint32_t process_id);
uint32_t label_get_file(uint32_t inode);
int label_check_access(uint32_t from_label, uint32_t to_label,
                      uint32_t operation);
const char *label_type_name(label_type_t type);
void label_get_stats(uint32_t *total_labels, uint32_t *proc_labels,
                    uint32_t *file_labels);

#endif /* SELEAF_LABELS_H */
