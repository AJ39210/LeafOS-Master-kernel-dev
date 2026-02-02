/**
 * kernel/SELeaf/namespaces.c
 * 
 * SELeaf Namespaces
 * 
 * Provides:
 * - Process namespace isolation
 * - Resource namespace
 * - IPC namespace
 * - Network namespace (simplified)
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Namespace types */
typedef enum {
    NS_PROCESS = 0,
    NS_RESOURCE,
    NS_IPC,
    NS_NETWORK
} namespace_type_t;

/* Namespace descriptor */
typedef struct {
    uint32_t ns_id;
    namespace_type_t type;
    uint32_t owner_pid;
    uint32_t member_count;
    uint32_t members[64];
} namespace_t;

/* Namespaces state */
#define NAMESPACES_MAX  32

typedef struct {
    namespace_t namespaces[NAMESPACES_MAX];
    uint32_t namespace_count;
    uint32_t isolations;
} ns_state_t;

static ns_state_t ns_state = {0};

/**
 * namespaces_init()
 */
int namespaces_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[NAMESPACES] Namespaces initialized\n");
    #endif
    ns_state.namespace_count = 0;
    ns_state.isolations = 0;
    return 0;
}

/**
 * namespace_create()
 */
uint32_t namespace_create(namespace_type_t type, uint32_t owner_pid)
{
    if (ns_state.namespace_count >= NAMESPACES_MAX) return 0;
    
    namespace_t *ns = &ns_state.namespaces[ns_state.namespace_count];
    ns->ns_id = ns_state.namespace_count + 1;
    ns->type = type;
    ns->owner_pid = owner_pid;
    ns->member_count = 1;
    ns->members[0] = owner_pid;
    
    ns_state.namespace_count++;
    return ns->ns_id;
}

/**
 * namespace_add_member()
 */
int namespace_add_member(uint32_t ns_id, uint32_t pid)
{
    if (ns_id == 0 || ns_id > ns_state.namespace_count) return -1;
    
    namespace_t *ns = &ns_state.namespaces[ns_id - 1];
    
    if (ns->member_count >= 64) return -1;
    
    ns->members[ns->member_count++] = pid;
    ns_state.isolations++;
    return 0;
}

/**
 * namespace_is_member()
 */
int namespace_is_member(uint32_t ns_id, uint32_t pid)
{
    if (ns_id == 0 || ns_id > ns_state.namespace_count) return 0;
    
    namespace_t *ns = &ns_state.namespaces[ns_id - 1];
    
    for (uint32_t i = 0; i < ns->member_count; i++) {
        if (ns->members[i] == pid) return 1;
    }
    
    return 0;
}

/**
 * namespace_get_stats()
 */
void namespace_get_stats(uint32_t *total, uint32_t *isolations)
{
    if (total) *total = ns_state.namespace_count;
    if (isolations) *isolations = ns_state.isolations;
}
