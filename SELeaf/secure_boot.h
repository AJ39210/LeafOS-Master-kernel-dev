/**
 * include/SELeaf/secure_boot.h
 */

#ifndef SELEAF_SECURE_BOOT_H
#define SELEAF_SECURE_BOOT_H

#include <stdint.h>

typedef enum {
    BOOT_STAGE_FIRMWARE = 0,
    BOOT_STAGE_BOOTLOADER,
    BOOT_STAGE_KERNEL,
    BOOT_STAGE_DRIVERS,
    BOOT_STAGE_USERSPACE
} boot_stage_t;

typedef enum {
    BOOT_STATUS_UNVERIFIED = 0,
    BOOT_STATUS_VERIFIED,
    BOOT_STATUS_FAILED,
    BOOT_STATUS_WARNING
} boot_status_t;

int secure_boot_init(void);
int secure_boot_measure(boot_stage_t stage, const uint8_t *data, uint32_t size);
int secure_boot_verify_signature(const uint8_t *data, uint32_t data_len,
                                 const uint8_t *signature, uint32_t sig_len);
int secure_boot_validate_chain(void);
int secure_boot_is_chain_valid(void);
int secure_boot_mark_stage(boot_stage_t stage, boot_status_t status);
void secure_boot_get_stats(uint32_t *measurements, uint32_t *violations, int *is_valid);
void secure_boot_dump_chain(void);
void secure_boot_disable(void);

#endif
