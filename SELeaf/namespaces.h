/**
 * include/SELeaf/namespaces.h
 */

#ifndef SELEAF_NAMESPACES_H
#define SELEAF_NAMESPACES_H

#include <stdint.h>

typedef enum {
    NS_PROCESS = 0,
    NS_RESOURCE,
    NS_IPC,
    NS_NETWORK
} namespace_type_t;

int namespaces_init(void);
uint32_t namespace_create(namespace_type_t type, uint32_t owner_pid);
int namespace_add_member(uint32_t ns_id, uint32_t pid);
int namespace_is_member(uint32_t ns_id, uint32_t pid);
void namespace_get_stats(uint32_t *total, uint32_t *isolations);

#endif
