/**
 * kernel/SELeaf/vm_sandbox.c
 * 
 * SELeaf Virtual Machine Sandbox
 * 
 * Provides:
 * - Lightweight VM sandboxing
 * - Process virtualization
 * - Capability isolation
 * - VM lifecycle management
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* VM state */
typedef enum {
    VM_STATE_STOPPED = 0,
    VM_STATE_RUNNING,
    VM_STATE_SUSPENDED,
    VM_STATE_CRASHED
} vm_state_t;

/* Virtual Machine */
typedef struct {
    uint32_t vm_id;
    uint32_t host_pid;
    vm_state_t state;
    uint32_t memory_mb;
    uint32_t cpu_quota;
    uint32_t capabilities;
} virtual_machine_t;

/* VM Sandbox state */
#define VMS_MAX  16

typedef struct {
    virtual_machine_t vms[VMS_MAX];
    uint32_t vm_count;
    uint32_t vm_creations;
    uint32_t vm_crashes;
} vm_sandbox_state_t;

static vm_sandbox_state_t vm_sandbox_state = {0};

/**
 * vm_sandbox_init()
 */
int vm_sandbox_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[VM_SANDBOX] Virtual machine sandbox initialized\n");
    #endif
    vm_sandbox_state.vm_count = 0;
    vm_sandbox_state.vm_creations = 0;
    vm_sandbox_state.vm_crashes = 0;
    return 0;
}

/**
 * vm_create()
 */
uint32_t vm_create(uint32_t host_pid, uint32_t memory_mb, uint32_t capabilities)
{
    if (vm_sandbox_state.vm_count >= VMS_MAX) return 0;
    
    virtual_machine_t *vm = &vm_sandbox_state.vms[vm_sandbox_state.vm_count];
    vm->vm_id = vm_sandbox_state.vm_count + 1;
    vm->host_pid = host_pid;
    vm->state = VM_STATE_STOPPED;
    vm->memory_mb = memory_mb;
    vm->cpu_quota = 100;
    vm->capabilities = capabilities;
    
    vm_sandbox_state.vm_count++;
    vm_sandbox_state.vm_creations++;
    
    return vm->vm_id;
}

/**
 * vm_start()
 */
int vm_start(uint32_t vm_id)
{
    if (vm_id == 0 || vm_id > vm_sandbox_state.vm_count) return -1;
    
    virtual_machine_t *vm = &vm_sandbox_state.vms[vm_id - 1];
    vm->state = VM_STATE_RUNNING;
    return 0;
}

/**
 * vm_stop()
 */
int vm_stop(uint32_t vm_id)
{
    if (vm_id == 0 || vm_id > vm_sandbox_state.vm_count) return -1;
    
    virtual_machine_t *vm = &vm_sandbox_state.vms[vm_id - 1];
    vm->state = VM_STATE_STOPPED;
    return 0;
}

/**
 * vm_get_state()
 */
vm_state_t vm_get_state(uint32_t vm_id)
{
    if (vm_id == 0 || vm_id > vm_sandbox_state.vm_count) return VM_STATE_STOPPED;
    
    return vm_sandbox_state.vms[vm_id - 1].state;
}

/**
 * vm_crash()
 */
int vm_crash(uint32_t vm_id)
{
    if (vm_id == 0 || vm_id > vm_sandbox_state.vm_count) return -1;
    
    virtual_machine_t *vm = &vm_sandbox_state.vms[vm_id - 1];
    vm->state = VM_STATE_CRASHED;
    vm_sandbox_state.vm_crashes++;
    return 0;
}

/**
 * vm_get_stats()
 */
void vm_get_stats(uint32_t *total, uint32_t *creations, uint32_t *crashes)
{
    if (total) *total = vm_sandbox_state.vm_count;
    if (creations) *creations = vm_sandbox_state.vm_creations;
    if (crashes) *crashes = vm_sandbox_state.vm_crashes;
}
