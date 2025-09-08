/**
 * @file bootloader.h
 * @brief Header file for bootloader state machine.
 */
#pragma once

#include <stdint.h>
#include "../data_comm/data_comm_rx.h"
#include "../btea/btea.h"
#include "./fw_verification/fw_verification.h"

#define BOOTLOADER_STATUS_OK 0x00
#define BOOTLOADER_ERROR_HEADER_VERIFICATION_FAILED 0x81
#define BOOTLOADER_ERROR_CRC_MISMATCH 0x82
#define BOOTLOADER_ERROR_VERIFICATION_FAILED 0x83
#define BOOTLOADER_ERROR_READ_ERROR 0x84
#define BOOTLOADER_ERROR_WRITE_ERROR 0x84
#define BOOTLOADER_ERROR_UNKNOWN 0xFF

typedef struct {
    uint32_t magic;
    fw_info_t info;
    uint32_t crc;
} bootloader_installed_fw_t;

typedef enum{
    BOOTLOADER_RUN_REQUEST_MODE_BOOTLOADER = 0,
    BOOTLOADER_RUN_REQUEST_MODE_APPLICATION = 1
} bootloader_run_request_mode_t;

typedef int (*bootloader_mem_write_t)(uint32_t address, const void *data, uint32_t size);
typedef int (*bootloader_mem_read_t)(uint32_t address, void* data, uint32_t size);
typedef int (*bootloader_mem_copy_t)(uint32_t src_address, uint32_t dst_address, uint32_t size);
typedef uint32_t (*bootloader_crc32_func_t)(uint32_t address, uint32_t size);
typedef void (*bootloader_jump_to_app_func_t)();
typedef data_comm_send_func_t bootloader_send_func_t;
typedef data_comm_log_func_t bootloader_log_func_t;

typedef struct {
    uint32_t address;
    uint32_t size;
}section_info_t;

typedef     struct {
    section_info_t app_info;
    section_info_t app;
    section_info_t upgrade_info;
    section_info_t upgrade;
} bootloader_sections_t;
typedef struct {
    const public_key_t* public_key;
    uint8_t device_id;
    const btea_key_t* btea_key;
    uint8_t *btea_buffer;
    uint32_t btea_chunk_size;
    struct {
        uint8_t padding_byte;
        uint32_t num_bytes;
    } padding;
    bootloader_mem_write_t mem_write_func;
    bootloader_mem_read_t mem_read_func;
    bootloader_mem_copy_t mem_copy_func;
    bootloader_crc32_func_t crc32_func;
    bootloader_send_func_t send_msg_func;
    bootloader_log_func_t log_func;
    bootloader_jump_to_app_func_t jump_to_app_func;
    uint32_t max_copy_retries;
    uint32_t jump_delay;
    uint32_t magic;
} bootloader_config_t;

/** Error type enumeration */
typedef enum {
    BOOT_ERROR_NONE,
    BOOT_ERROR_MULTIPLE_CRC_RECEPTION,
    BOOT_ERROR_PACKET_CRC_RECEIVE_TIMEOUT,
    BOOT_ERROR_PACKET_CRC_MISMATCH,
    BOOT_ERROR_SEQUENCE_ERROR,
    BOOT_ERROR_INVALID_HEADER,
    BOOT_ERROR_FLASH_WRITE_FAILED,
    BOOT_ERROR_UNKNOWN
} bootloader_error_t;


/**
 * @brief Executes the bootloader state machine.
 *
 * This function should be called periodically to run the bootloader logic.
 */
void bootloader_init(const bootloader_config_t* bootloader_config, const bootloader_sections_t* sections);
void bootloader_tick(uint32_t time_ms);
void bootloader_start_app( bool jump);
void bootloader_stay(bool stay);
void bootloader_rx_message_received(uint32_t time_ms, data_comm_msg_type_t type, uint8_t id, const uint8_t* data, uint16_t size);
uint8_t bootloader_app_status();
uint32_t bootloader_get_installed_fw_version(void);
bool bootloader_is_receiving();