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
#define MEM_APP_INFO_END_ADDRESS            0x08004000

/* Application Area */
#define MEM_APP_START_ADDRESS               0x08004000
#define MEM_APP_END_ADDRESS                 0x08076000

/* Upgrade Information */
#define MEM_UPGRADE_INFO_ADDRESS            0x08076000
#define MEM_UPGRADE_INFO_END_ADDRESS        0x08078000

/* Upgrade Area */
#define MEM_UPGRADE_START_ADDRESS           0x08078000
#define MEM_UPGRADE_END_ADDRESS             0x080EA000

/* Configuration 1 Area */
#define MEM_CONFIG_1_START_ADDRESS          0x080EA000
#define MEM_CONFIG_1_END_ADDRESS            0x080EC000

/* Configuration 2 Area */
#define MEM_CONFIG_2_START_ADDRESS          0x080EC000
#define MEM_CONFIG_2_END_ADDRESS            0x080EE000

/* EOL Information Area */
#define MEM_EOL_INFO_START_ADDRESS          0x080EE000
#define MEM_EOL_INFO_END_ADDRESS            0x080F0000

/* Reserved Area */
#define MEM_RESERVED_START_ADDRESS          0x080F0000
#define MEM_RESERVED_END_ADDRESS          	0x080F8000

/* Bootloader Area */
#define MEM_BOOTLOADER_START_ADDRESS        0x080F8000
#define MEM_BOOTLOADER_END_ADDRESS          0x08100000

/* End of FLASH Memory */
#define MEM_FLASH_END_ADDRESS               0x08100000


void mem_init( void );
int mem_read( uint32_t address, void* data, uint32_t size);
int mem_copy(uint32_t src_address, uint32_t dst_address, uint32_t size);
int mem_write(uint32_t address, const void *data, uint32_t size);
