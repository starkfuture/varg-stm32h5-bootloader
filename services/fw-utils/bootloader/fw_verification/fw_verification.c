#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <tinycrypt/ecc_dsa.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>
#include "fw.h"
#include "fw_verification.h"
// Firmware header structure


// Verify the firmware header's signature using the public key
int fw_verification_process_header(const fw_header_t* header, const public_key_t* public_key) {

    if (header==NULL) {
        return -2;
    }

    uint8_t header_hash[32];
    struct tc_sha256_state_struct sha_ctx;
    const struct uECC_Curve_t * curve = uECC_secp256r1();

    // Hash the header (excluding the signature fields)
    tc_sha256_init(&sha_ctx);
    tc_sha256_update(&sha_ctx, (uint8_t *)header, sizeof(fw_header_t) - sizeof(header->signature));
    tc_sha256_final(header_hash, &sha_ctx);

    // Verify the ECDSA signature
    if (uECC_verify(public_key->key, header_hash, sizeof(header_hash), header->signature,  curve)) {
        return 0;  // Signature valid
    } else {
        return -1; // Signature invalid
    }
}

int fw_verification_process_init(fw_t* firmware, const fw_header_t* header, const public_key_t* public_key) {
    if (firmware==NULL || header==NULL) {
        return -2;
    }
    int result = fw_verification_process_header(header,public_key);
    if (result!=0) {
        return result;
    }
    tc_sha256_init(&firmware->sha_ctx);
    firmware->header=header;
    firmware->bytes_processed=0;

    return 0;
}

// Process binary data in chunks
int fw_verification_process_binary(fw_t* firmware, uint8_t* data, uint32_t size) {
    if (firmware==NULL || firmware->header==NULL || data==NULL) {
        return -1;  // Null pointer error
    }

    // Ensure the size doesn't exceed the remaining firmware size
    if (firmware->bytes_processed + size > firmware->header->fw_info.fw_size) {
        return -2;  // Out of bounds
    }

    // Update the cumulative SHA-256 hash
    tc_sha256_update(&firmware->sha_ctx, data, size);
    firmware->bytes_processed += size;

    // Check if the entire binary has been processed
    if (firmware->bytes_processed == firmware->header->fw_info.fw_size) {
        uint8_t final_hash[32];
        tc_sha256_final(final_hash, &firmware->sha_ctx);

        // Verify the final hash matches the one in the header
        if (memcmp(final_hash, firmware->header->fw_hash, 32) == 0) {
            return 1;  // Firmware successfully processed
        } else {
            return -3;  // Hash mismatch
        }
    }

    return 0;  // Processing is incomplete but no errors
}



