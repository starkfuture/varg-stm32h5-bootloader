/**
 * @file mem.h
 * @brief Hardware Abstraction Layer of FLASH
 * @details This file provides the interface definitions for the FLASH peripheral operations.
 *          It includes functions for initialization, reading, writing, copying, and erasing FLASH memory.
 *          The addresses for various FLASH regions are also defined here.
 * @author Marc Ros Iglesias
 * @date 26/06/2025
 */

#pragma once

/* Includes ------------------------------------------------------------------*/

/* FLASH Memory Map -----------------------------------------------------------*/
/* Application Information */
#define MEM_APP_INFO_ADDRESS                0x08002000
#define MEM_APP_START_ADDRESS               0x08004000

/* Upgrade Information */
#define MEM_UPGRADE_INFO_ADDRESS            0x08076000
#define MEM_UPGRADE_START_ADDRESS           0x08078000

/* Configuration Banks */
#define MEM_CONFIGURATION_1_ADDRESS         0x080EA000
#define MEM_CONFIGURATION_2_ADDRESS         0x080EC000

/* End-of-Line Information */
#define MEM_EOL_INFO_ADDRESS                0x080EE000

/* Reserved Area */
#define MEM_RESERVED_ADDRESS                0x080F0000

/* Bootloader Area */
#define MEM_BOOTLOADER_ADDRESS              0x080F8000

/* End of FLASH Memory */
#define MEM_FLASH_END_ADDRESS               0x08100000

void mem_init( void );
int mem_read( uint32_t address, void* data, uint32_t size);
int mem_copy(uint32_t src_address, uint32_t dst_address, uint32_t size);
int mem_write(uint32_t address, const void *data, uint32_t size);
