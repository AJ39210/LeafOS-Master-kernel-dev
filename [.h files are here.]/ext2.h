#ifndef EXT2_H
#define EXT2_H

#include <stdint.h>

#define EXT2_MAGIC 0xEF53
#define EXT2_BLOCK_SIZE 4096

typedef struct {
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t reserved_blocks;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t block_size;
    uint32_t fragment_size;
    uint32_t blocks_per_group;
    uint32_t fragments_per_group;
    uint32_t inodes_per_group;
    uint32_t mount_time;
    uint32_t write_time;
    uint16_t mount_count;
    uint16_t max_mount_count;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t version_minor;
    uint32_t last_check;
    uint32_t check_interval;
    uint32_t creator_os;
    uint32_t version_major;
    uint16_t uid_reserved;
    uint16_t gid_reserved;
} __attribute__((packed)) ext2_superblock_t;

typedef struct {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t access_time;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t deletion_time;
    uint16_t gid;
    uint16_t link_count;
    uint32_t block_count;
    uint32_t flags;
    uint32_t os_specific_1;
    uint32_t block_pointers[12];
    uint32_t indirect_block;
    uint32_t double_indirect;
    uint32_t triple_indirect;
    uint32_t generation_number;
    uint32_t file_acl;
    uint32_t directory_acl;
    uint32_t fragment_address;
    uint8_t os_specific_2[12];
} __attribute__((packed)) ext2_inode_t;

int ext2_mount(void);
int ext2_unmount(void);
int ext2_read_file(const char *filename, void *buffer, uint32_t size);
int ext2_list_directory(void);

#endif
