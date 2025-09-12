/**
  ******************************************************************************
  * @file    sf_charger_external_adc_hal.h
  * @brief   Interface for ADS8638 ADC using blocking SPI (STM32H5 HAL-based).
  *
  * Provides initialization, configuration of input range and channel reading
  * for the Texas Instruments ADS8638 analog-to-digital converter.
  *
  * SPI communication is performed in blocking mode using HAL SPI functions.
  * Chip Select is managed manually using a GPIO pin.
  *
  * @author  [Your Name]
  * @date    2025
  ******************************************************************************
  */


#include <stdint.h>

/* General defines ------------------------------------------------------------------*/

/**
 * @brief Number of ADC conversions
 */
#define EXTERNAL_ADC_CONVERSIONS	3

/**
 * @brief Enum to identify each conversion
 */
typedef enum {
	VAC1 = 0,
	VAC2,
	CAN_GND2PE,
} external_adc_enum_t;


void external_adc_init(void);
void get_external_adc_conversions(float adc_conversions []);


