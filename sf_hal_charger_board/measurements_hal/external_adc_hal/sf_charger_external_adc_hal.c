/**
  ******************************************************************************
  * @file    sf_charger_external_adc_hal.c
  * @brief   Source file for ADS8638 ADC driver using blocking SPI (STM32H5 HAL).
  *
  * This module handles SPI communication and channel/range configuration
  * for the ADS8638 external ADC, with support for basic power control.
  *
  * SPI must be configured in:
  * - Mode 3 (CPOL=1, CPHA=1)
  * - 16-bit data size
  * - MSB first
  * - Software NSS (manual CS handling)
  *
  * Example:
  *   sf_adc_init();
  *   uint16_t value = sf_adc_read_channel(3);
  *
  * Required definitions:
  *   #define SF_ADC_SPI_HANDLE    hspi2
  *   #define SF_ADC_CS_GPIO_Port  GPIOG
  *   #define SF_ADC_CS_Pin        GPIO_PIN_7
  *
  * @author  [Your Name]
  * @date    2025
  ******************************************************************************
  */

#include "sf_charger_external_adc_hal.h"
#include "sf_spi_hal.h"
#include "sf_gpio_hal.h"


/* General defines (pinout mapping) ------------------------------------------------------------------*/
/**
 * @brief definition of the peripheral number where the external ADC is connected
 */
#define SPI_ADC_PERIPHERAL			2

/**
 * @brief definition of the peripheral number where the external ADC is connected
 */
#define SPI_ADC_CS					gpio_B12

/**
 * @brief GPIO that allows the SPI communication (enables the output level shifter)
 */
#define ENABLE_SPI					gpio_G7

/**
 * @Brief Mapping of the VAC1 ADCC channel
 */
#define VAC1_ADC_CHANNEL			(uint8_t) 5

/**
 * @Brief Mapping of the VAC2 ADC channel
 */
#define VAC2_ADC_CHANNEL			(uint8_t) 6

/**
 * @Brief Mapping of the CAN GND to PE ADC channel
 */
#define CAN_GND2PE__ADC_CHANNEL		(uint8_t) 1

/* Private functions ------------------------------------------------------------------*/

/**
 * @brief Transmit a 16-bit command word via SPI to ADS8638.
 * @param cmd 16-bit command
 */
static void sf_adc_write_command(uint8_t address, uint8_t config)
{
	uint16_t spi_tx_data = (address << 9) | (0 << 8) | config;
	gpio_clear(SPI_ADC_CS);
    uint16_t dummy_rx;
    spi_transmit_receive(SPI_ADC_PERIPHERAL, (uint8_t*)&spi_tx_data, (uint8_t*)&dummy_rx, 1);
    gpio_set(SPI_ADC_CS);
}

/* Public functions ------------------------------------------------------------------*/

/**
 * @brief Function to recover the 3 conversions that the external ADC is doing
 * @param adc_conversions, array where the results must be stored
 * @Note the pointer/array that is used in the function must have at least @ref EXTERNAL_ADC_CONVERSIONS positions
 */
void get_external_adc_conversions(float adc_conversions [])
{
	// Auxiliary variables to read all the conversions
	uint16_t spi_rx_aux = 0;
	uint16_t spi_tx_aux = 0;
	uint16_t channel = 0xFF;
	uint16_t adc_conversion = 0;
	float adc_pin_voltage = 0;
	uint8_t i = 0;

	// Send as many dummy messages to the configured channels
	for (i = 0; i < EXTERNAL_ADC_CONVERSIONS; i++){
		gpio_clear(SPI_ADC_CS);
		spi_transmit_receive(SPI_ADC_PERIPHERAL, (uint8_t*)&spi_tx_aux, (uint8_t*)&spi_rx_aux, 1);
		gpio_set(SPI_ADC_CS);

		channel = (spi_rx_aux >> 12) & 0x0F;
		adc_conversion = spi_rx_aux &  0xFFF;

		// Scaling by considering the +-2.5 V range. Therefore each conversion will have a 0x800h offset at 0 V (2^12/2) and then the scaling factor will be 2.5/2^11
		adc_pin_voltage = (float) ((int16_t) adc_conversion - 0x800) *  0.001220703125f;

		if (channel == VAC1_ADC_CHANNEL){
			adc_conversions[VAC1] = adc_pin_voltage;
		} else if (channel == VAC2_ADC_CHANNEL){
			adc_conversions[VAC2] = adc_pin_voltage;
		} else if (channel == CAN_GND2PE__ADC_CHANNEL){
			adc_conversions[CAN_GND2PE] = adc_pin_voltage;
		} else {
			// DO NOTHING
		}

	}

}

/**
 * @brief Initialize the ADS8638
 * @details Power-up, configure input ranges, define the conversion sequence.
 */
void external_adc_init(void)
{

	// The SPI should be already configured from the CubeMX, so no initialization of the peripheral is required

	// Enable level shifter for the SPI communications
	gpio_set(ENABLE_SPI);

	// Power-up of the ADC
	sf_adc_write_command((uint8_t) 0x04, (uint8_t) 0x00);

	// Turn on the internal reference
	sf_adc_write_command((uint8_t) 0x06, (uint8_t) 0x0C);
	sf_adc_write_command((uint8_t) 0x06, (uint8_t) 0x0C);

	// Configure all the channels as +-2.5V range
	sf_adc_write_command((uint8_t) 0x10, (uint8_t) 0x33);			// Channels 0 and 1
	sf_adc_write_command((uint8_t) 0x11, (uint8_t) 0x33);			// Channels 2 and 3
	sf_adc_write_command((uint8_t) 0x12, (uint8_t) 0x33);			// Channels 4 and 5
	sf_adc_write_command((uint8_t) 0x13, (uint8_t) 0x33);			// Channels 6 and 7

	// Configure the sequence conversions
	// CH1 --> CAN_GND3PE
	// CH5 --> Vac1
	// CH6 --> Vac2
	sf_adc_write_command((uint8_t) 0x0C, (uint8_t) (1 << 6) | (1 << 2) | (1<<1) );

	// Reset the sequence
	sf_adc_write_command((uint8_t) 0x05, (uint8_t) 0x80);

}



