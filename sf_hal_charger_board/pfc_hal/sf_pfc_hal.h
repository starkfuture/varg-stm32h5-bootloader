/**
 * @file sf_pfc_hal.h
 * @brief Hardware Abstraction Layer interface to manage the PFC of the charger
 * @note Check that the project build configuration includes this path (Properties > C/C++ Build > Settings > Tool Settings > MCU/MPU GCC Compiler > Includes).
 * 			If not, add ../sf_hal_charger_board/pfc_hal. Also check that this folder is not excluded from Build (Right click on scheduler folder > Properties > C/C++ Build > Exclude Resource from Build).
 * @author Lucas Bouzon Pousa
 * @date 28/05/2024
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
	PFC_OK,					/*!<Indicate the PFC has no failures*/
	PFC_NOK,				/*!<Indicate the PFC is not working*/
	PFC_WARNING,			/*!<Indicate the PFC is not working with limited performance (the available power will be less than 1 pu)*/
} pfc_status_t;

/**
 * @brief Structure to configure the possible interrupts of the peripheral
 */
typedef struct {
	pfc_status_t	status;  								/*!<Indicate the status of the PFC (@ref pfc_status_t)*/
	uint8_t			wroking_branches;						/*!<Indicate how many working branches the PFC has at the moment*/
	float 			available_power;						/*!<Indicate the total available power of the PFC in pus.*/
} pfc_info_t;

/* Public Functions ------------------------------------------------------------------*/
void init_pfc (float pfc_switching_freq_khz, float pfc_current_control_it_freq_khz, float pfc_voltage_control_it_freq_khz);
void set_pfc_callback_function (void (*callback_function)(void));
void stop_pfc (void);
void enable_pfc (void);
void set_pfc_duty (float duty_pu);
void set_pfc_frequency (float frequency_khz);
float get_pfc_available_power (void);
pfc_info_t get_pfc_info (void);
