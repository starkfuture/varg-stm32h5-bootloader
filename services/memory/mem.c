/**
 * @file sf_flash_hal.c
 * @brief Hardware Abstraction Layer of FLASH
 * @details This file provides the implementation of functions to handle FLASH operations.
 *          The implementation uses STM32H5 HAL and provides functionality for reading,
 *          writing, erasing and copying FLASH sectors.
 * @author Marc Ros Iglesias
 * @date 26/06/2025
 */

/* Global Includes ------------------------------------------------------------------*/

/* Private Includes ------------------------------------------------------------------*/
#include "sf_flash_hal.h"
#include "nand_flash.h"
#include "mem.h"

/* Private defines ------------------------------------------------------------------*/

/* Static Variables -----------------------------------------------------------------*/
static nand_t nand;

void mem_init( void )
{
	/* Initialize nand_t structure with pointers to implementation functions */
	nand.base_address    = 0;
	nand.check_func      = sf_flash_check;
	nand.context         = NULL;
	nand.erase_page_func = sf_flash_erase_page;
	nand.get_page_func   = sf_flash_get_page_index;
	nand.log_func        = NULL;
	nand.min_size_write  = MEM_FLASH_WRITE_ALIGNMENT;
	nand.read_func       = sf_flash_read;
	nand.write_func      = sf_flash_write;
}

int mem_read( uint32_t address, void* data, uint32_t size) {

	if (address >= MEM_FLASH_END_ADDRESS || (address +size)>=MEM_FLASH_END_ADDRESS) {
        return -1;
    }

    return sf_flash_read(address, data, size, NULL);
}

int mem_copy(uint32_t src_address, uint32_t dst_address,uint32_t size) {
	uint8_t status = nand_write_erase(&nand,dst_address,(const void*)src_address,size,true);
	if (status == NAND_STATUS_SUCCESS) {
		return 0;
	} else {
		return status;
	}
}

int mem_write(uint32_t address, const void *data,uint32_t size) {

	uint8_t status = nand_write_erase(&nand,address,data,size,true);

	if (status == NAND_STATUS_SUCCESS) {
		return 0;
	} else {
		return status;
	}
}
