//
// Created by nico on 1/11/18.
//

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define NAND_LOG_LEVEL_DEBUG   0x00
#define NAND_LOG_LEVEL_INFO    0x01
#define NAND_LOG_LEVEL_WARNING 0x02
#define NAND_LOG_LEVEL_ERROR   0x03
#ifndef NAND_LOG_LEVEL
#define NAND_LOG_LEVEL 0xFF //Disable log by default
#endif

#define NAND_CRC_SEED   0xDEADBEEF

#define NAND_STATUS_SUCCESS 0x00
#define NAND_ERROR_WRITING 0x01
#define NAND_ERROR_READING 0x02
#define NAND_ERROR_ERASING 0x03
#define NAND_ERROR_MOVING 0x04
#define NAND_ERROR_OOB 0x05
#define NAND_ERROR_NO_BUFFER 0x06
#define NAND_ERROR_CRC_MISMATCH 0x07
#define NAND_ERROR_INSUFICIENT_BUFFER 0x08
#define NAND_ERROR_INVALID_PARAM 0x09

typedef struct {
    uint8_t * buffer;
    uint32_t buffer_size;
    uint32_t size;
    uint32_t index;
    uint32_t start_address;
}nand_page_t;
typedef int (*nand_log_func_t)(int level, const char *format, ...);

typedef bool (*nand_get_page_func_t) (uint32_t address, nand_page_t* page,void* context);
typedef uint8_t (*nand_r_func_t) (uint32_t address, uint8_t* data, uint32_t size, void *context);
typedef uint8_t (*nand_w_func_t) (uint32_t address, const uint8_t* data, uint32_t size, void *context);
typedef uint8_t (*nand_erase_page_func_t) (uint32_t page_index, uint32_t page_start_addr, uint32_t page_size,void* context);
typedef bool (*nand_check_write_t) (uint8_t flash_data, uint8_t new_data);
typedef struct {
    nand_get_page_func_t get_page_func;
    nand_w_func_t write_func;
    nand_r_func_t read_func;
    nand_erase_page_func_t erase_page_func;
    nand_log_func_t log_func;
    nand_check_write_t check_func;
    uint32_t base_address;
    uint32_t min_size_write;
    void *context;
} nand_t;


uint8_t nand_write_erase(const nand_t *nand, uint32_t address, const uint8_t *data, uint32_t size, bool always_backup);


uint8_t nand_move(const nand_t *nand, uint32_t src_addr, uint32_t dst_addr, uint32_t size);

void nand_init_irregular_pages(nand_page_t *page, const uint32_t start_address, const uint32_t page_count, const uint32_t *sizes, uint8_t *const *buffers);
void nand_init_pages(nand_page_t *page, const uint32_t start_address, const uint32_t page_count, uint32_t page_size, uint8_t* const page_buffer);
