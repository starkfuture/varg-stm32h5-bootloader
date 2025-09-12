/**
 * @file sf_buck_hal.h
 * @brief Hardware Abstraction Layer interface to manage the Buck converter of the charger
 * @note Check that the project build configuration includes this path (Properties > C/C++ Build > Settings > Tool Settings > MCU/MPU GCC Compiler > Includes).
 * 			If not, add ../sf_hal_charger_board/buck_hal. Also check that this folder is not excluded from Build (Right click on scheduler folder > Properties > C/C++ Build > Exclude Resource from Build).
 * @author Lucas Bouzon Pousa
 * @date 05/06/2024
 */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h> 		// Standard types
#include <stddef.h>  		// Definition of NULL

/* Public definitions ------------------------------------------------------------------*/

/**
 * @brief Enumeration to define the possible states of the PFC
 */
typedef enum {
	BUCK_OK,				/*!<Indicate the Buck has no failures*/
	BUCK_NOK,				/*!<Indicate the Buck is not working*/
	BUCK_WARNING,			/*!<Indicate the Buck is not working with limited performance (the available power will be less than 1 pu)*/
} buck_status_t;

/**
 * @brief Structure to define the information of the buck converter (copied from the PFC)
 */
typedef struct {
	buck_status_t	status;  								/*!<Indicate the status of the Buck (@ref buck_status_t)*/
	uint8_t			wroking_branches;						/*!<Indicate how many working branches the buck has at the moment*/
	float 			available_power;						/*!<Indicate the total available power of the buck.*/
} buck_info_t;



/* Public Functions ------------------------------------------------------------------*/
void init_buck (float buck_switching_freq_khz);
void set_buck_callback_function (void (*callback_function)(void));
void stop_buck (void);
void enable_buck (void);
void set_buck_duty (float duty_pu);
void set_buck_frequency (float frequency_khz);
buck_info_t get_buck_info (void);
