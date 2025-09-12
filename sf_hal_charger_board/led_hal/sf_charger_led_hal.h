/**
 * @file sf_charger_led_hal.h
 * @brief LED abstraction layer
 * @details  Mapping of the LEDs to the GPIOs
 * @note Check that the project build configuration includes this path (Properties > C/C++ Build > Settings > Tool Settings > MCU/MPU GCC Compiler > Includes).
 * 			If not, add ../sf_hal_charger_board/led_hal. Also check that this folder is not excluded from Build (Right click on scheduler folder > Properties > C/C++ Build > Exclude Resource from Build).
 * @author Lucas Bouzon Pousa
 * @date 28/05/2024
 */

/* Includes ------------------------------------------------------------------*/
#pragma once
#include "sf_gpio_hal.h"

/* General defines ------------------------------------------------------------------*/
/**
 * @brief GPIO of the LED1
*/
#define LED1	     gpio_D9
/**
 * @brief GPIO of the LED1
*/
#define LED2		 gpio_D10

/**
 * @brief GPIO for debug timings (this is mapped where the RCD_meas)
*/
#define DEBUG_PIN		 gpio_C1
