#include "ram.h"
#include "serial.h"
#include <stddef.h>

/* Real memory map from BIOS/bootloader */
static memory_map_t system_memory = {0};

/* Simple hex to string conversion */
static void uint64_to_hex(uint64_t value, char *str) {
    const char *hex = "0123456789ABCDEF";
    for (int i = 15; i >= 0; i--) {
        str[i] = hex[value & 0xF];
        value >>= 4;
    }
    str[16] = '\0';
}

void ram_driver_init(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[RAM] RAM Driver Initializing...\n");
    #endif
    
    system_memory.total_memory = 0;
    system_memory.usable_memory = 0;
    system_memory.reserved_memory = 0;
    system_memory.num_entries = 0;
}

void ram_detect_memory(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[RAM] Detecting system memory...\n");
    #endif
    
    /* Simulated BIOS e820 memory map (realistic for QEMU/typical system) */
    e820_entry_t mem_map[] = {
        {0x000000, 0x09F000, E820_RAM_USABLE, 0},           /* 639KB usable */
        {0x09F000, 0x061000, E820_RAM_RESERVED, 0},         /* 384KB reserved (EBDA + BIOS) */
        {0x0E0000, 0x020000, E820_RAM_RESERVED, 0},         /* 128KB reserved (video memory) */
        {0x100000, 0x7F00000, E820_RAM_USABLE, 0},          /* 127MB usable */
        {0x8000000, 0x8000000, E820_RAM_USABLE, 0},         /* 128MB usable (high memory) */
    };
    
    int map_count = sizeof(mem_map) / sizeof(e820_entry_t);
    system_memory.num_entries = map_count;
    
    for (int i = 0; i < map_count; i++) {
        system_memory.entries[i] = mem_map[i];
        
        if (mem_map[i].type == E820_RAM_USABLE) {
            system_memory.usable_memory += mem_map[i].length;
        }
        system_memory.total_memory += mem_map[i].length;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[RAM] Memory detection complete\n");
    #endif
}

uint64_t ram_get_total_memory(void) {
    return system_memory.total_memory;
}

uint64_t ram_get_usable_memory(void) {
    return system_memory.usable_memory;
}

void ram_print_memory_map(void) {
    for (uint32_t i = 0; i < system_memory.num_entries; i++) {
        e820_entry_t *entry = &system_memory.entries[i];
        uint64_t end_addr = entry->base_address + entry->length;
        
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[RAM] [mem 0x");
        
        /* Print base address */
        for (int j = 12; j >= 0; j--) {
            uint8_t nibble = (entry->base_address >> (j * 4)) & 0xF;
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[nibble]);
        }
        
        serial_puts(SERIAL_PORT_A, "-0x");
        
        /* Print end address */
        for (int j = 12; j >= 0; j--) {
            uint8_t nibble = (end_addr >> (j * 4)) & 0xF;
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[nibble]);
        }
        
        serial_puts(SERIAL_PORT_A, "] ");
        
        switch (entry->type) {
            case E820_RAM_USABLE:
                serial_puts(SERIAL_PORT_A, "usable\n");
                break;
            case E820_RAM_RESERVED:
                serial_puts(SERIAL_PORT_A, "reserved\n");
                break;
            case E820_RAM_ACPI_RECLAIM:
                serial_puts(SERIAL_PORT_A, "ACPI reclaim\n");
                break;
            case E820_RAM_ACPI_NVS:
                serial_puts(SERIAL_PORT_A, "ACPI NVS\n");
                break;
            case E820_RAM_UNUSABLE:
                serial_puts(SERIAL_PORT_A, "unusable\n");
                break;
            default:
                serial_puts(SERIAL_PORT_A, "type:");
                serial_putchar(SERIAL_PORT_A, "0123456789"[entry->type]);
                serial_puts(SERIAL_PORT_A, "\n");
        }
        #endif
    }
}

int ram_get_memory_map(e820_entry_t *entries, int max_entries) {
    if (!entries || max_entries <= 0) return -1;
    
    int count = (max_entries < system_memory.num_entries) ? max_entries : system_memory.num_entries;
    for (int i = 0; i < count; i++) {
        entries[i] = system_memory.entries[i];
    }
    
    return count;
}
