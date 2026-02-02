/**
 * include/SELeaf/syscall_hooks.h
 */

#ifndef SELEAF_SYSCALL_HOOKS_H
#define SELEAF_SYSCALL_HOOKS_H

#include <stdint.h>

int syscall_hooks_init(void);
int syscall_hook_entry(uint32_t syscall_num, uint32_t *args, uint32_t current_pid);
void syscall_hook_exit(uint32_t syscall_num, int result, uint32_t current_pid);
int syscall_hook_file_open(uint32_t current_pid, const char *file_path, int flags);
int syscall_hook_file_read(uint32_t current_pid, uint32_t fd, uint32_t size);
int syscall_hook_file_write(uint32_t current_pid, uint32_t fd, uint32_t size);
int syscall_hook_process_create(uint32_t parent_pid, uint32_t new_pid);
void syscall_hook_process_exit(uint32_t exiting_pid);
int syscall_hook_network_connect(uint32_t current_pid, uint32_t remote_ip, uint16_t port);
int syscall_hook_memory_allocate(uint32_t current_pid, uint32_t size_kb);
int syscall_hook_signal(uint32_t from_pid, uint32_t to_pid, uint32_t signal);
void syscall_hooks_get_stats(uint32_t *total, uint32_t *blocked, 
                            uint32_t *audited, uint32_t *violations);
void syscall_hooks_enable_enforcement(void);
void syscall_hooks_disable_enforcement(void);

#endif
