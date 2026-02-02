#include "vfs.h"
#include "serial.h"
#include <stddef.h>

#define MAX_OPEN_FILES 32
#define MAX_FILESYSTEMS 8

/* Simple VFS file descriptor table */
typedef struct {
    int in_use;
    uint32_t inode;
    uint32_t position;
    uint32_t size;
} vfs_file_descriptor_t;

static vfs_file_descriptor_t open_files[MAX_OPEN_FILES];
static vfs_stats_t vfs_stats = {0};

/* Simulated root filesystem */
static const vfs_inode_t root_entries[] = {
    {1, VFS_TYPE_DIRECTORY, 0755, 4096, 0, 0, 0, "/"},
    {2, VFS_TYPE_DIRECTORY, 0755, 4096, 0, 0, 0, "boot"},
    {3, VFS_TYPE_DIRECTORY, 0755, 4096, 0, 0, 0, "dev"},
    {4, VFS_TYPE_DIRECTORY, 0755, 4096, 0, 0, 0, "proc"},
    {5, VFS_TYPE_DIRECTORY, 0755, 4096, 0, 0, 0, "sys"},
    {6, VFS_TYPE_DIRECTORY, 0755, 4096, 0, 0, 0, "etc"},
    {7, VFS_TYPE_DIRECTORY, 0755, 4096, 0, 0, 0, "home"},
    {8, VFS_TYPE_DIRECTORY, 0755, 4096, 0, 0, 0, "var"},
};

void vfs_driver_init(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[VFS] Virtual File System Driver Initializing...\n");
    #endif
    
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        open_files[i].in_use = 0;
    }
    
    vfs_stats.num_filesystems = 1;
    vfs_stats.total_size = 256 * 1024 * 1024;    /* 256MB virtual */
    vfs_stats.used_size = 0;
    vfs_stats.free_size = vfs_stats.total_size;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[VFS] VFS initialized with 256MB virtual storage\n");
    #endif
}

void vfs_mount_root(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[VFS] Mounting root filesystem\n");
    serial_puts(SERIAL_PORT_A, "[VFS] Root mounted at /\n");
    #endif
}

int vfs_get_stats(vfs_stats_t *stats) {
    if (!stats) return -1;
    
    *stats = vfs_stats;
    return 0;
}

int vfs_open(const char *path) {
    if (!path) return -1;
    
    /* Find free file descriptor */
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_files[i].in_use) {
            open_files[i].in_use = 1;
            open_files[i].inode = 1;
            open_files[i].position = 0;
            open_files[i].size = 4096;
            
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[VFS] Opened file: ");
            serial_puts(SERIAL_PORT_A, path);
            serial_puts(SERIAL_PORT_A, " (fd=");
            serial_putchar(SERIAL_PORT_A, "0123456789"[i]);
            serial_puts(SERIAL_PORT_A, ")\n");
            #endif
            
            return i;
        }
    }
    
    return -1;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return -1;
    
    if (open_files[fd].in_use) {
        open_files[fd].in_use = 0;
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[VFS] Closed file descriptor ");
        serial_putchar(SERIAL_PORT_A, "0123456789"[fd]);
        serial_puts(SERIAL_PORT_A, "\n");
        #endif
        return 0;
    }
    
    return -1;
}

int vfs_read(int fd, void *buffer, uint32_t size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !buffer) return -1;
    if (!open_files[fd].in_use) return -1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[VFS] Read: fd=");
    serial_putchar(SERIAL_PORT_A, "0123456789"[fd]);
    serial_puts(SERIAL_PORT_A, " size=");
    serial_putchar(SERIAL_PORT_A, "0123456789"[(size / 10) % 10]);
    serial_putchar(SERIAL_PORT_A, "0123456789"[size % 10]);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return size;
}

int vfs_write(int fd, void *buffer, uint32_t size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !buffer) return -1;
    if (!open_files[fd].in_use) return -1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[VFS] Write: fd=");
    serial_putchar(SERIAL_PORT_A, "0123456789"[fd]);
    serial_puts(SERIAL_PORT_A, " size=");
    serial_putchar(SERIAL_PORT_A, "0123456789"[(size / 10) % 10]);
    serial_putchar(SERIAL_PORT_A, "0123456789"[size % 10]);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    vfs_stats.used_size += size;
    if (vfs_stats.free_size > size) {
        vfs_stats.free_size -= size;
    }
    
    return size;
}

int vfs_list_directory(const char *path) {
    if (!path) return -1;
    
    int entry_count = sizeof(root_entries) / sizeof(vfs_inode_t);
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[VFS] Directory listing for ");
    serial_puts(SERIAL_PORT_A, path);
    serial_puts(SERIAL_PORT_A, ":\n");
    
    for (int i = 0; i < entry_count; i++) {
        serial_puts(SERIAL_PORT_A, "[VFS]   ");
        serial_puts(SERIAL_PORT_A, root_entries[i].name);
        serial_puts(SERIAL_PORT_A, "\n");
    }
    #endif
    
    return entry_count;
}
