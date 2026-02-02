#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

#define FAT32_SECTOR_SIZE 4096

typedef struct {
    uint8_t jmp[3];
    uint8_t oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entries;
    uint16_t total_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_large;
    uint32_t fat_size_32;
    uint16_t extended_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
} __attribute__((packed)) fat32_boot_sector_t;

typedef struct {
    char filename[8];
    char extension[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t high_cluster;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t low_cluster;
    uint32_t file_size;
} __attribute__((packed)) fat32_directory_entry_t;

int fat32_mount(void);
int fat32_unmount(void);
int fat32_read_file(const char *filename, void *buffer, uint32_t size);
int fat32_list_directory(void);

#endif
