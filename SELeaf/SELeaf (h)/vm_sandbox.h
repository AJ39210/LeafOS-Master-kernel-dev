/**
 * include/SELeaf/vm_sandbox.h
 */

#ifndef SELEAF_VM_SANDBOX_H
#define SELEAF_VM_SANDBOX_H

#include <stdint.h>

typedef enum {
    VM_STATE_STOPPED = 0,
    VM_STATE_RUNNING,
    VM_STATE_SUSPENDED,
    VM_STATE_CRASHED
} vm_state_t;

int vm_sandbox_init(void);
uint32_t vm_create(uint32_t host_pid, uint32_t memory_mb, uint32_t capabilities);
int vm_start(uint32_t vm_id);
int vm_stop(uint32_t vm_id);
vm_state_t vm_get_state(uint32_t vm_id);
int vm_crash(uint32_t vm_id);
void vm_get_stats(uint32_t *total, uint32_t *creations, uint32_t *crashes);

#endif
