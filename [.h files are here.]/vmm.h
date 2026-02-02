/**
 * include/vmm.h
 * 
 * Virtual Memory Manager (VMM)
 * 
 * Provides:
 * - Page directory and table management
 * - Virtual to physical address translation
 * - Memory protection (RW, User/Kernel)
 * - TLB management
 */

#ifndef VMM_H
#define VMM_H

#include <stdint.h>

/* Page size */
#define PAGE_SIZE       0x1000      /* 4KB */

/* Protection flags */
#define PAGE_PRESENT    0x00000001  /* Page present in memory */
#define PAGE_RW         0x00000002  /* Read/Write */
#define PAGE_USER       0x00000004  /* User accessible */
#define PAGE_GLOBAL     0x00000100  /* Global (not flushed by CR3 reload) */

/* Virtual memory zones */
#define KERNEL_BASE     0xC0000000  /* Kernel space (3GB+) */
#define USER_BASE       0x08048000  /* User space (default) */

/**
 * vmm_init()
 * Initialize virtual memory system
 * Called during kernel startup
 */
void vmm_init(void);

/**
 * vmm_vaddr_to_paddr()
 * Translate virtual address to physical address
 * Returns: physical address (or 0 if unmapped)
 */
uint32_t vmm_vaddr_to_paddr(uint32_t vaddr);

/**
 * vmm_map_page()
 * Map virtual page to physical page
 * Returns: 0 on success, -1 on error
 */
int vmm_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags);

/**
 * vmm_unmap_page()
 * Unmap virtual page
 * Returns: 0 on success, -1 on error
 */
int vmm_unmap_page(uint32_t vaddr);

/**
 * vmm_is_mapped()
 * Check if virtual address is currently mapped
 * Returns: 1 if mapped, 0 if not
 */
int vmm_is_mapped(uint32_t vaddr);

/**
 * vmm_get_stats()
 * Get memory statistics
 */
void vmm_get_stats(uint32_t *total, uint32_t *used);

#endif // VMM_H
