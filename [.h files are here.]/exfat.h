#ifndef EXFAT_H
#define EXFAT_H

#include <stdint.h>

#define EXFAT_SECTOR_SIZE 4096

typedef struct {
    uint8_t jmp[3];
    uint8_t oem[8];
    uint8_t reserved[53];
    uint64_t partition_offset;
    uint64_t volume_length;
    uint32_t fat_offset;
    uint32_t fat_length;
    uint32_t cluster_heap_offset;
    uint32_t cluster_count;
    uint32_t root_directory_cluster;
    uint32_t serial_number;
    uint16_t file_system_revision;
    uint16_t volume_flags;
    uint8_t bytes_per_sector_shift;
    uint8_t sectors_per_cluster_shift;
    uint8_t number_of_fats;
} __attribute__((packed)) exfat_boot_sector_t;

typedef struct {
    char filename[256];
    uint32_t attributes;
    uint32_t first_cluster;
    uint64_t file_size;
    uint32_t created;
    uint32_t modified;
} exfat_file_entry_t;

int exfat_mount(void);
int exfat_unmount(void);
int exfat_read_file(const char *filename, void *buffer, uint32_t size);
int exfat_list_directory(void);

#endif
