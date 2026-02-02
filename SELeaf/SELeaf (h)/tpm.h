/*
 * SELeaf TPM (Trusted Platform Module) Header
 */

#ifndef SELEAF_TPM_H
#define SELEAF_TPM_H

#include <stdint.h>

/* Maximum numbers */
#define TPM_MAX_PCRS                    24      /* Platform Configuration Registers */
#define TPM_MAX_KEYS                    32      /* Cryptographic keys */
#define TPM_MAX_SEALED_SIZE             4096    /* Max sealed data size */
#define TPM_MAX_QUOTE_SIZE              1024    /* Max quote size */

/* TPM Vendors */
#define TPM_VENDOR_INFINEON             0x15D1
#define TPM_VENDOR_ATMEL                0x1114
#define TPM_VENDOR_NUVOTON              0x00EB

/* TPM Capabilities */
#define TPM_CAP_SHA1                    (1 << 0)
#define TPM_CAP_SHA256                  (1 << 1)
#define TPM_CAP_RSA                     (1 << 2)
#define TPM_CAP_AES                     (1 << 3)
#define TPM_CAP_HMAC                    (1 << 4)
#define TPM_CAP_ECC                     (1 << 5)

/* Key Types */
#define TPM_KEY_RSA                     1
#define TPM_KEY_AES                     2
#define TPM_KEY_ECC                     3
#define TPM_KEY_HMAC                    4

/* Key Flags */
#define TPM_KEY_FLAG_SIGN               (1 << 0)
#define TPM_KEY_FLAG_ENCRYPT            (1 << 1)
#define TPM_KEY_FLAG_EXPORTABLE         (1 << 2)
#define TPM_KEY_FLAG_FIXED_PARENT       (1 << 3)

/* Key Handles */
#define TPM_SRK_HANDLE                  0x40000000  /* Storage Root Key */
#define TPM_KEY_HANDLE_BASE             0x80000000

/* PCR Status */
#define TPM_PCR_LOCKED                  (1 << 0)
#define TPM_PCR_RESETABLE               (1 << 1)

/* TPM Status Structure */
typedef struct {
    uint32_t device_vendor;
    uint32_t device_id;
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t activated;
    uint8_t initialized;
    uint32_t capabilities;
    uint32_t total_keys;
    uint32_t total_operations;
    uint32_t total_errors;
} tpm_status_t;

/* Function declarations */
int32_t tpm_init(void);

int32_t tpm_extend_pcr(uint8_t pcr_index, const uint8_t *measurement,
                       uint32_t measurement_size);

int32_t tpm_quote_pcrs(uint32_t quote_nonce, uint32_t selected_pcrs,
                       uint8_t *quote_data, uint32_t max_size);

int32_t tpm_create_key(uint8_t key_type, uint32_t key_size,
                       uint32_t key_flags);

int32_t tpm_sign_data(uint32_t key_handle, const uint8_t *data,
                      uint32_t data_size, uint8_t *signature,
                      uint32_t max_sig_size);

int32_t tpm_seal_data(const uint8_t *data, uint32_t data_size,
                      uint32_t pcr_mask);

int32_t tpm_unseal_data(uint8_t *output_data, uint32_t max_size);

void tpm_get_status(tpm_status_t *status);

int32_t tpm_get_pcr(uint8_t pcr_index, uint8_t *pcr_value);

int32_t tpm_lock_pcr(uint8_t pcr_index);

uint32_t tpm_get_capabilities(void);

#endif /* SELEAF_TPM_H */
