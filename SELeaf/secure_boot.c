/**
 * kernel/SELeaf/secure_boot.c
 * 
 * SELeaf Secure Boot Chain
 * 
 * Provides:
 * - Bootloader verification
 * - Kernel signature validation
 * - Boot integrity chain
 * - Measured boot with hashes
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Boot stage identification */
typedef enum {
    BOOT_STAGE_FIRMWARE = 0,
    BOOT_STAGE_BOOTLOADER,
    BOOT_STAGE_KERNEL,
    BOOT_STAGE_DRIVERS,
    BOOT_STAGE_USERSPACE
} boot_stage_t;

/* Boot verification status */
typedef enum {
    BOOT_STATUS_UNVERIFIED = 0,
    BOOT_STATUS_VERIFIED,
    BOOT_STATUS_FAILED,
    BOOT_STATUS_WARNING
} boot_status_t;

/* Boot measurement (PCR-like) */
typedef struct {
    uint32_t hash[8];           /* SHA256 hash (simplified) */
    uint32_t size;
    boot_stage_t stage;
    boot_status_t status;
} boot_measurement_t;

/* Boot chain state */
#define BOOT_MEASUREMENTS_MAX   16

typedef struct {
    boot_measurement_t measurements[BOOT_MEASUREMENTS_MAX];
    uint32_t measurement_count;
    
    int secure_boot_enabled;
    int chain_valid;
    uint32_t violations;
    uint32_t last_stage;
} secure_boot_state_t;

static secure_boot_state_t boot_state = {0};

/**
 * secure_boot_init()
 * Initialize secure boot system
 */
int secure_boot_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SECURE_BOOT] Secure boot initialized\n");
    #endif
    
    boot_state.measurement_count = 0;
    boot_state.secure_boot_enabled = 1;
    boot_state.chain_valid = 1;
    boot_state.violations = 0;
    boot_state.last_stage = BOOT_STAGE_FIRMWARE;
    
    return 0;
}

/**
 * secure_boot_measure()
 * Add measurement to boot chain (PCR-like)
 */
int secure_boot_measure(boot_stage_t stage, const uint8_t *data, uint32_t size)
{
    if (boot_state.measurement_count >= BOOT_MEASUREMENTS_MAX) {
        boot_state.chain_valid = 0;
        return -1;
    }
    
    boot_measurement_t *meas = &boot_state.measurements[boot_state.measurement_count];
    
    /* Simple hash (XOR-based for now) */
    uint32_t hash_val = 0;
    for (uint32_t i = 0; i < size; i++) {
        hash_val ^= ((uint32_t)data[i] << (8 * (i % 4)));
    }
    
    meas->hash[0] = hash_val;
    for (int i = 1; i < 8; i++) {
        meas->hash[i] = hash_val * (i + 1);
    }
    
    meas->size = size;
    meas->stage = stage;
    meas->status = BOOT_STATUS_VERIFIED;
    
    boot_state.measurement_count++;
    boot_state.last_stage = stage;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SECURE_BOOT] Measured stage ");
    serial_putchar(SERIAL_PORT_A, '0' + stage);
    serial_puts(SERIAL_PORT_A, " (");
    
    char size_str[16];
    uint32_t temp = size;
    int len = 0;
    if (temp == 0) {
        size_str[len++] = '0';
    } else {
        while (temp > 0) {
            size_str[15 - len] = '0' + (temp % 10);
            temp /= 10;
            len++;
        }
    }
    if (len > 0) {
        serial_puts(SERIAL_PORT_A, &size_str[15 - len + 1]);
    }
    
    serial_puts(SERIAL_PORT_A, " bytes)\n");
    #endif
    
    return 0;
}

/**
 * secure_boot_verify_signature()
 * Verify cryptographic signature (framework)
 * 
 * Returns: 1 if valid, 0 if invalid
 */
int secure_boot_verify_signature(const uint8_t *data, uint32_t data_len,
                                 const uint8_t *signature, uint32_t sig_len)
{
    (void)data;
    (void)data_len;
    (void)signature;
    (void)sig_len;
    
    /* Framework for RSA/ECDSA signature verification
       In production: use libgcrypt or mbedTLS */
    
    /* For now: accept all (replace with real crypto) */
    return 1;
}

/**
 * secure_boot_validate_chain()
 * Validate entire boot chain integrity
 * 
 * Returns: 1 if valid, 0 if compromised
 */
int secure_boot_validate_chain(void)
{
    if (!boot_state.secure_boot_enabled) {
        return 1;  /* Disabled */
    }
    
    if (boot_state.measurement_count < 2) {
        boot_state.chain_valid = 0;
        return 0;  /* Incomplete chain */
    }
    
    /* Check measurement sequence validity */
    for (uint32_t i = 1; i < boot_state.measurement_count; i++) {
        boot_measurement_t *prev = &boot_state.measurements[i-1];
        boot_measurement_t *curr = &boot_state.measurements[i];
        
        /* Verify stage progression */
        if (curr->stage <= prev->stage) {
            boot_state.violations++;
            boot_state.chain_valid = 0;
            return 0;  /* Invalid stage order */
        }
        
        /* Verify status */
        if (prev->status != BOOT_STATUS_VERIFIED) {
            boot_state.violations++;
            boot_state.chain_valid = 0;
            return 0;  /* Previous stage not verified */
        }
    }
    
    return 1;  /* Chain valid */
}

/**
 * secure_boot_is_chain_valid()
 * Check if boot chain is currently valid
 */
int secure_boot_is_chain_valid(void)
{
    return boot_state.chain_valid;
}

/**
 * secure_boot_mark_stage()
 * Mark a boot stage as completed/verified
 */
int secure_boot_mark_stage(boot_stage_t stage, boot_status_t status)
{
    if (boot_state.measurement_count == 0) {
        return -1;
    }
    
    boot_measurement_t *meas = &boot_state.measurements[boot_state.measurement_count - 1];
    if (meas->stage == stage) {
        meas->status = status;
        
        if (status == BOOT_STATUS_FAILED) {
            boot_state.chain_valid = 0;
            boot_state.violations++;
        }
        
        return 0;
    }
    
    return -1;
}

/**
 * secure_boot_get_stats()
 * Get boot chain statistics
 */
void secure_boot_get_stats(uint32_t *measurements, uint32_t *violations, int *is_valid)
{
    if (measurements) *measurements = boot_state.measurement_count;
    if (violations) *violations = boot_state.violations;
    if (is_valid) *is_valid = boot_state.chain_valid;
}

/**
 * secure_boot_dump_chain()
 * Display boot chain measurements
 */
void secure_boot_dump_chain(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "\n[SECURE_BOOT] Boot Chain Report\n");
    serial_puts(SERIAL_PORT_A, "================================\n");
    
    for (uint32_t i = 0; i < boot_state.measurement_count; i++) {
        boot_measurement_t *m = &boot_state.measurements[i];
        
        serial_puts(SERIAL_PORT_A, "Stage ");
        serial_putchar(SERIAL_PORT_A, '0' + m->stage);
        serial_puts(SERIAL_PORT_A, ": ");
        
        switch (m->status) {
            case BOOT_STATUS_VERIFIED:
                serial_puts(SERIAL_PORT_A, "VERIFIED");
                break;
            case BOOT_STATUS_FAILED:
                serial_puts(SERIAL_PORT_A, "FAILED");
                break;
            case BOOT_STATUS_WARNING:
                serial_puts(SERIAL_PORT_A, "WARNING");
                break;
            default:
                serial_puts(SERIAL_PORT_A, "UNVERIFIED");
        }
        
        serial_puts(SERIAL_PORT_A, " (");
        
        char size_str[16];
        uint32_t temp = m->size;
        int len = 0;
        if (temp == 0) {
            size_str[len++] = '0';
        } else {
            while (temp > 0) {
                size_str[15 - len] = '0' + (temp % 10);
                temp /= 10;
                len++;
            }
        }
        if (len > 0) {
            serial_puts(SERIAL_PORT_A, &size_str[15 - len + 1]);
        }
        
        serial_puts(SERIAL_PORT_A, " bytes)\n");
    }
    
    serial_puts(SERIAL_PORT_A, "Chain Valid: ");
    serial_puts(SERIAL_PORT_A, boot_state.chain_valid ? "YES" : "NO");
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
}

/**
 * secure_boot_disable()
 * Disable secure boot (for development)
 */
void secure_boot_disable(void)
{
    boot_state.secure_boot_enabled = 0;
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[SECURE_BOOT] Secure boot DISABLED\n");
    #endif
}
