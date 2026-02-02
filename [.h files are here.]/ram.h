#ifndef RAM_H
#define RAM_H

#include <stdint.h>

/* Memory Map Entry */
typedef struct {
    uint64_t base_address;
    uint64_t length;
    uint32_t type;           /* 1=available, 2=reserved, etc */
    uint32_t acpi_extended;
} __attribute__((packed)) e820_entry_t;

#define E820_RAM_USABLE       1
#define E820_RAM_RESERVED     2
#define E820_RAM_ACPI_RECLAIM 3
#define E820_RAM_ACPI_NVS     4
#define E820_RAM_UNUSABLE     5

typedef struct {
    uint64_t total_memory;
    uint64_t usable_memory;
    uint64_t reserved_memory;
    uint32_t num_entries;
    e820_entry_t entries[32];
} memory_map_t;

void ram_driver_init(void);
void ram_detect_memory(void);
uint64_t ram_get_total_memory(void);
uint64_t ram_get_usable_memory(void);
void ram_print_memory_map(void);
int ram_get_memory_map(e820_entry_t *entries, int max_entries);

#endif
