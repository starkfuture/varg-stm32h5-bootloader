/**
 * @file sf_charger_relays_hal.h
 * @brief Relay abstraction layer
 * @details  Mapping of the Relays to the GPIOs
 * @note Check that the project build configuration includes this path (Properties > C/C++ Build > Settings > Tool Settings > MCU/MPU GCC Compiler > Includes).
 * 			If not, add ../sf_hal_charger_board/relay_hal. Also check that this folder is not excluded from Build (Right click on scheduler folder > Properties > C/C++ Build > Exclude Resource from Build).
 * @author Lucas Bouzon Pousa
 * @date 05/06/2024
 */

/* Includes ------------------------------------------------------------------*/
#pragma once
#include "sf_gpio_hal.h"

/* General defines ------------------------------------------------------------------*/
/**
 * @brief GPIO of the input relay (the one in parallel to the pre-charge resistor)
*/
#define INPUT_RELAY	     	gpio_F7

/**
 * @brief GPIO of both output relays
*/
#define OUTPUT_RELAY	     gpio_F8
