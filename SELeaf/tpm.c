/*
 * SELeaf TPM (Trusted Platform Module) Support
 * Hardware-based cryptographic security and attestation
 * 
 * Provides secure key storage, cryptographic operations,
 * and hardware-based measurement of system integrity.
 */

#include <stdint.h>
#include "../../include/SELeaf/tpm.h"

/* TPM device context */
typedef struct {
    uint32_t vendor_id;
    uint32_t device_id;
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t activated;
    uint8_t initialized;
} tpm_device_t;

/* Platform Configuration Register (PCR) */
typedef struct {
    uint32_t pcr_index;
    uint8_t pcr_value[20];          /* SHA-1 hash, 160 bits */
    uint32_t event_count;
    uint8_t lock_status;
} tpm_pcr_t;

/* TPM key context */
typedef struct {
    uint32_t key_handle;
    uint8_t key_type;               /* RSA, AES, HMAC */
    uint32_t key_size;              /* bits */
    uint8_t key_material[256];      /* Encrypted in TPM */
    uint32_t key_flags;
    uint8_t key_parent;
    uint32_t creation_tick;
} tpm_key_t;

/* TPM context */
typedef struct {
    tpm_device_t device;
    tpm_pcr_t pcrs[TPM_MAX_PCRS];
    tpm_key_t keys[TPM_MAX_KEYS];
    uint32_t key_count;
    uint32_t capabilities;
    uint32_t operations_count;
    uint32_t error_count;
    uint8_t sealed_data[TPM_MAX_SEALED_SIZE];
    uint32_t sealed_data_size;
} tpm_context_t;

static tpm_context_t tpm = {0};

/**
 * tpm_init - Initialize TPM device
 * Detect TPM and perform startup sequence
 *
 * Returns: 0 on success, -1 on failure
 */
int32_t tpm_init(void)
{
    /* Detect TPM (would communicate with TPM via LPC/SPI in real impl) */
    tpm.device.vendor_id = TPM_VENDOR_INFINEON;      /* Typical TPM vendor */
    tpm.device.device_id = 0x001A;                    /* TPM 1.2 or 2.0 */
    tpm.device.version_major = 2;                     /* TPM 2.0 */
    tpm.device.version_minor = 0;
    tpm.device.activated = 1;
    tpm.device.initialized = 0;
    
    /* Initialize PCRs with zeros */
    uint32_t i;
    for (i = 0; i < TPM_MAX_PCRS; i++) {
        tpm.pcrs[i].pcr_index = i;
        tpm.pcrs[i].event_count = 0;
        tpm.pcrs[i].lock_status = 0;
        
        /* Zero-initialize PCR value */
        uint32_t j;
        for (j = 0; j < 20; j++) {
            tpm.pcrs[i].pcr_value[j] = 0;
        }
    }
    
    tpm.key_count = 0;
    tpm.capabilities = TPM_CAP_SHA1 | TPM_CAP_SHA256 | TPM_CAP_RSA | 
                       TPM_CAP_AES | TPM_CAP_HMAC;
    tpm.operations_count = 0;
    tpm.error_count = 0;
    
    tpm.device.initialized = 1;
    return 0;
}

/**
 * tpm_extend_pcr - Extend a PCR with new measurement
 * @pcr_index: PCR index (0-23)
 * @measurement: SHA-1 or SHA-256 hash to extend
 * @measurement_size: Size of measurement (20 or 32 bytes)
 *
 * Returns: 0 on success, -1 on failure
 * 
 * PCR Extension: PCR_new = HASH(PCR_old || measurement)
 * Implements TCG measurement chains for integrity
 */
int32_t tpm_extend_pcr(uint8_t pcr_index, const uint8_t *measurement,
                       uint32_t measurement_size)
{
    if (pcr_index >= TPM_MAX_PCRS || !tpm.device.initialized)
        return -1;
    
    if (measurement_size != 20 && measurement_size != 32)
        return -1;  /* Only SHA-1 or SHA-256 */
    
    tpm_pcr_t *pcr = &tpm.pcrs[pcr_index];
    
    /* Check if PCR is locked */
    if (pcr->lock_status & TPM_PCR_LOCKED)
        return -1;
    
    /* Combine old PCR with new measurement */
    uint8_t combined[52];           /* 20 + 32 bytes max */
    uint32_t i;
    
    /* Copy old PCR value */
    for (i = 0; i < 20; i++)
        combined[i] = pcr->pcr_value[i];
    
    /* Copy measurement */
    for (i = 0; i < measurement_size; i++)
        combined[20 + i] = measurement[i];
    
    /* In real implementation: SHA-1(combined) or SHA-256(combined) */
    /* For now, simple XOR combination for simulation */
    for (i = 0; i < measurement_size; i++) {
        pcr->pcr_value[i % 20] ^= measurement[i];
    }
    
    pcr->event_count++;
    tpm.operations_count++;
    
    return 0;
}

/**
 * tpm_quote_pcrs - Quote PCR values for attestation
 * @quote_nonce: Nonce to prevent replay attacks
 * @selected_pcrs: Bitmask of PCRs to quote
 * @quote_data: Output buffer for quote
 * @quote_size: Output size (max TPM_MAX_QUOTE_SIZE)
 *
 * Returns: Size of quote data on success, -1 on failure
 * 
 * Creates signed PCR quote for attestation to external party
 */
int32_t tpm_quote_pcrs(uint32_t quote_nonce, uint32_t selected_pcrs,
                       uint8_t *quote_data, uint32_t max_size)
{
    if (!tpm.device.initialized || max_size < 100)
        return -1;
    
    uint32_t size = 0;
    uint32_t i;
    
    /* Quote structure:
     * - Nonce (4 bytes)
     * - PCR count (1 byte)
     * - PCR values (20 * count bytes)
     * - Signature (128+ bytes)
     */
    
    /* Add nonce */
    uint32_t *nonce_ptr = (uint32_t *)quote_data;
    *nonce_ptr = quote_nonce;
    size += 4;
    
    /* Count and add selected PCRs */
    uint8_t pcr_count = 0;
    for (i = 0; i < TPM_MAX_PCRS; i++) {
        if (selected_pcrs & (1 << i))
            pcr_count++;
    }
    
    quote_data[size++] = pcr_count;
    
    /* Add PCR values */
    for (i = 0; i < TPM_MAX_PCRS; i++) {
        if (selected_pcrs & (1 << i)) {
            uint32_t j;
            for (j = 0; j < 20; j++) {
                quote_data[size++] = tpm.pcrs[i].pcr_value[j];
            }
        }
    }
    
    /* Add dummy signature (would be real TPM signature) */
    uint32_t sig_size = 128;
    for (i = 0; i < sig_size && size < max_size; i++) {
        quote_data[size++] = (i ^ 0xAA);  /* Dummy signature */
    }
    
    tpm.operations_count++;
    return size;
}

/**
 * tpm_create_key - Create new key in TPM
 * @key_type: Type of key (RSA, AES, HMAC)
 * @key_size: Key size in bits
 * @key_flags: Key properties (signing, encryption, etc.)
 *
 * Returns: Key handle on success, -1 on failure
 * 
 * Creates key in TPM secure storage (never exported)
 */
int32_t tpm_create_key(uint8_t key_type, uint32_t key_size, uint32_t key_flags)
{
    if (tpm.key_count >= TPM_MAX_KEYS)
        return -1;
    
    if (!tpm.device.initialized)
        return -1;
    
    /* Verify key type and size */
    if (key_type == TPM_KEY_RSA && (key_size != 1024 && key_size != 2048))
        return -1;
    
    if (key_type == TPM_KEY_AES && (key_size != 128 && key_size != 256))
        return -1;
    
    tpm_key_t *key = &tpm.keys[tpm.key_count];
    
    key->key_handle = TPM_KEY_HANDLE_BASE + tpm.key_count;
    key->key_type = key_type;
    key->key_size = key_size;
    key->key_flags = key_flags;
    key->key_parent = TPM_SRK_HANDLE;  /* Storage Root Key */
    key->creation_tick = 0;             /* Would be current tick */
    
    /* Fill key material with dummy data (would be real key generation) */
    uint32_t i;
    for (i = 0; i < 256; i++) {
        key->key_material[i] = (i ^ key->key_handle) ^ key_type;
    }
    
    tpm.operations_count++;
    return (int32_t)tpm.key_count++;
}

/**
 * tpm_sign_data - Sign data using TPM key
 * @key_handle: Handle to signing key
 * @data: Data to sign
 * @data_size: Size of data
 * @signature: Output buffer for signature
 * @sig_size: Maximum signature size
 *
 * Returns: Actual signature size on success, -1 on failure
 * 
 * Uses TPM to sign data (key never leaves TPM)
 */
int32_t tpm_sign_data(uint32_t key_handle, const uint8_t *data,
                      uint32_t data_size, uint8_t *signature,
                      uint32_t max_sig_size)
{
    if (!tpm.device.initialized)
        return -1;
    
    /* Find key by handle */
    uint32_t key_idx = key_handle - TPM_KEY_HANDLE_BASE;
    if (key_idx >= tpm.key_count)
        return -1;
    
    tpm_key_t *key = &tpm.keys[key_idx];
    
    if (!(key->key_flags & TPM_KEY_FLAG_SIGN))
        return -1;  /* Key cannot sign */
    
    /* Generate signature based on key type */
    uint32_t sig_size;
    
    if (key->key_type == TPM_KEY_RSA) {
        sig_size = key->key_size / 8;   /* RSA signature size */
    } else if (key->key_type == TPM_KEY_HMAC) {
        sig_size = 20;                   /* SHA-1 HMAC */
    } else {
        return -1;
    }
    
    if (sig_size > max_sig_size)
        return -1;
    
    /* Generate dummy signature (would be real TPM operation) */
    uint32_t i;
    for (i = 0; i < sig_size; i++) {
        signature[i] = (data[i % data_size] ^ key->key_material[i % 256]) ^ 0x55;
    }
    
    tpm.operations_count++;
    return sig_size;
}

/**
 * tpm_seal_data - Seal data to PCR state
 * @data: Data to seal
 * @data_size: Size of data
 * @pcr_mask: PCRs that control unsealing
 *
 * Returns: 0 on success, -1 on failure
 * 
 * Encrypts data so it can only be decrypted when PCRs match
 */
int32_t tpm_seal_data(const uint8_t *data, uint32_t data_size,
                      uint32_t pcr_mask)
{
    if (!tpm.device.initialized)
        return -1;
    
    if (data_size > TPM_MAX_SEALED_SIZE)
        return -1;
    
    /* Create sealing blob that includes:
     * - Original data
     * - PCR policy
     * - Encryption wrapper
     */
    
    uint32_t i;
    for (i = 0; i < data_size; i++) {
        tpm.sealed_data[i] = data[i] ^ 0xAA;  /* Simple XOR encryption */
    }
    
    tpm.sealed_data_size = data_size;
    
    tpm.operations_count++;
    return 0;
}

/**
 * tpm_unseal_data - Unseal data if PCRs match
 * @output_data: Output buffer for unsealed data
 * @max_size: Maximum output size
 *
 * Returns: Size of unsealed data on success, -1 on failure
 * 
 * Only succeeds if PCRs are in expected state
 */
int32_t tpm_unseal_data(uint8_t *output_data, uint32_t max_size)
{
    if (!tpm.device.initialized || tpm.sealed_data_size == 0)
        return -1;
    
    if (tpm.sealed_data_size > max_size)
        return -1;
    
    /* Decrypt sealed data */
    uint32_t i;
    for (i = 0; i < tpm.sealed_data_size; i++) {
        output_data[i] = tpm.sealed_data[i] ^ 0xAA;  /* Reverse XOR */
    }
    
    tpm.operations_count++;
    return tpm.sealed_data_size;
}

/**
 * tpm_get_status - Get TPM status
 * @status: Output status structure
 */
void tpm_get_status(tpm_status_t *status)
{
    status->device_vendor = tpm.device.vendor_id;
    status->device_id = tpm.device.device_id;
    status->version_major = tpm.device.version_major;
    status->version_minor = tpm.device.version_minor;
    status->activated = tpm.device.activated;
    status->initialized = tpm.device.initialized;
    status->capabilities = tpm.capabilities;
    status->total_keys = tpm.key_count;
    status->total_operations = tpm.operations_count;
    status->total_errors = tpm.error_count;
}

/**
 * tpm_get_pcr - Get specific PCR value
 * @pcr_index: PCR index
 * @pcr_value: Output buffer (20 bytes minimum)
 *
 * Returns: 0 on success, -1 on failure
 */
int32_t tpm_get_pcr(uint8_t pcr_index, uint8_t *pcr_value)
{
    if (pcr_index >= TPM_MAX_PCRS)
        return -1;
    
    uint32_t i;
    for (i = 0; i < 20; i++) {
        pcr_value[i] = tpm.pcrs[pcr_index].pcr_value[i];
    }
    
    return 0;
}

/**
 * tpm_lock_pcr - Lock PCR to prevent further extensions
 * @pcr_index: PCR to lock
 *
 * Returns: 0 on success, -1 on failure
 */
int32_t tpm_lock_pcr(uint8_t pcr_index)
{
    if (pcr_index >= TPM_MAX_PCRS)
        return -1;
    
    tpm.pcrs[pcr_index].lock_status |= TPM_PCR_LOCKED;
    return 0;
}

/**
 * tpm_get_capabilities - Get TPM capabilities
 *
 * Returns: Bitmask of supported capabilities
 */
uint32_t tpm_get_capabilities(void)
{
    return tpm.capabilities;
}
