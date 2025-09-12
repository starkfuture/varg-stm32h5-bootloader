/**
 * @file sf_charger_measurements_hal.h
 * @brief Measurements abstraction layer interface
 * @details  Interface to access all the measurements of the charger
 * @note Check that the project build configuration includes this path (Properties > C/C++ Build > Settings > Tool Settings > MCU/MPU GCC Compiler > Includes).
 * 			If not, add ../sf_hal_charger_board/measurements_hal. Also check that this folder is not excluded from Build (Right click on scheduler folder > Properties > C/C++ Build > Exclude Resource from Build).
 * @author Lucas Bouzon Pousa
 * @date 05/06/2024
 */


#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h> 		// Standard types
#include <math.h>			// Floor types


/* General Definitions ------------------------------------------------------------------*/

/**
 * @brief Mapping to the timer that will be used to measure ns times internally
 * @Note it is defiend in the .h because it should be able to be access from many apps
 */
#define NS_TIMER_PERIPHERAL_NUMBER	(uint8_t) 14

/**
 * @brief Enumeration to define the possible errors for the measurements
 *
 * This enumeration is used to handle and describe the different status that can happen
 * in the analog measurements.
 */
typedef enum {
    MEASUREMENT_OK = 1,  			/*!<Measurement OK, no problem encountered */
    MASUREMENT_OUTOFRANGE = 2,		/*!<Measurement out of the configured range */
	MASUREMENT_ERROR = 3, 			/*!<Measurement has an error */
} measurement_status_t;

/**
 * @brief Enumeration to define all the high speed measurements
 * @Note the order is super important. The ADC channels (ADC1 or ADC2) should be ordered depending on the ruting, first the ones that are routed to the lower number channel
 */
typedef enum {
	/*HIGH SPEED ADC1 */
    OUT_CURRENT 	= 0,
	BUS_VOLTAGE,
	OUT_VOLTAGE,
	IN_CURRENT,
	/*FAST MEASUREMENT EXTERNAL ADC (SPI)*/
	AC1_VOLTAGE,
	AC2_VOLTAGE,
	CAN2PE_VOLTAGE,
	/*LOW SPEED ADC2*/
	POWERPCB_TEMP,
	BAT_VOLTAGE,
	HW_VERSION,
	CONTROLPCB_TEMP,
	SUPPLY_24V,
	/*MCU INTERNAL MEASUREMENTS */
	MCU_TEMP,
	VREF_INT,
	VBAT_INT,
	VDD_CORE,
	TOTAL_MEASUREMENTS,
} measurements_t;

/**
 * @brief Total fast measurements
 * @note it is important to have this define consistent with the @ref measruements_t and the @ref get_high_speed_measurements function.
 */
#define	TOTAL_FAST_MEASUREMENTS	POWERPCB_TEMP

/* Public Functions ------------------------------------------------------------------ */
measurement_status_t init_measurements (void);
measurement_status_t start_low_speed_conversions (void);
measurement_status_t get_high_speed_measurements (float measurements []);
measurement_status_t get_internal_adc_high_speed_measurements (float measurements []);
measurement_status_t get_all_measurements (float measurements []);
float get_input_current (void);
measurement_status_t set_measurements_callback_function (void (*callback_function)(void));
