#ifndef VFS_H
#define VFS_H

#include <stdint.h>

/* File Types */
#define VFS_TYPE_FILE       1
#define VFS_TYPE_DIRECTORY  2
#define VFS_TYPE_SYMLINK    3
#define VFS_TYPE_BLOCKDEV   4
#define VFS_TYPE_CHARDEV    5

/* File Permissions */
#define VFS_PERM_READ       4
#define VFS_PERM_WRITE      2
#define VFS_PERM_EXECUTE    1

typedef struct {
    uint32_t inode;
    uint16_t type;
    uint16_t permissions;
    uint32_t size;
    uint32_t created;
    uint32_t modified;
    uint32_t accessed;
    char name[256];
} vfs_inode_t;

typedef struct {
    uint32_t num_filesystems;
    uint64_t total_size;
    uint64_t used_size;
    uint64_t free_size;
} vfs_stats_t;

void vfs_driver_init(void);
void vfs_mount_root(void);
int vfs_get_stats(vfs_stats_t *stats);
int vfs_open(const char *path);
int vfs_close(int fd);
int vfs_read(int fd, void *buffer, uint32_t size);
int vfs_write(int fd, void *buffer, uint32_t size);
int vfs_list_directory(const char *path);

#endif
