#pragma once
#include "fw.h"
#include "tinycrypt/sha256.h"

// Structure to manage firmware processing
typedef struct {
    uint32_t bytes_processed;   // Number of bytes processed
    struct tc_sha256_state_struct sha_ctx;  // Cumulative SHA-256 context
    const fw_header_t *header;        // Verified firmware header
} fw_t;

typedef struct {
    uint8_t key[64];
} public_key_t;

typedef struct {
    uint8_t key[32];
} private_key_t;


int fw_verification_process_header(const fw_header_t* header, const public_key_t* public_key);
int fw_verification_process_init(fw_t* firmware, const fw_header_t* header, const public_key_t* public_key);
int fw_verification_process_binary(fw_t* firmware, uint8_t* data, uint32_t size);

