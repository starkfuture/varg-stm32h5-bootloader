//
// Created by nico on 11/10/24.
//

#pragma once

#include <stdint.h>


#pragma pack(push,1)

typedef struct {
    uint32_t fw_size;       // Firmware size in bytes
    uint32_t fw_crc;        // Firmware CRC
    uint32_t fw_version;    // Firmware version
    uint8_t flags;          // Firmware flags
    uint8_t info[19];       // Firmware info
} fw_info_t;

typedef struct {
    uint8_t fw_hash[32];    // SHA-256 of the firmware
    fw_info_t fw_info;      // Firmware info structure
    uint8_t signature[64];  // ECDSA signature part
} fw_header_t;

#pragma pack(pop)
