/**
 * @file sf_charger_measurements_hal.c
 * @brief Measurements abstraction layer
 * @details All needed public and private functions needed to get all the measurements of the charger
 * @note Check that the project build configuration includes this path (Properties > C/C++ Build > Settings > Tool Settings > MCU/MPU GCC Compiler > Includes).
 * 			If not, add ../sf_hal_charger_board/measurements_hal. Also check that this folder is not excluded from Build (Right click on scheduler folder > Properties > C/C++ Build > Exclude Resource from Build).
 * @author Lucas Bouzon Pousa
 * @date 05/06/2024
 */

/* Includes ------------------------------------------------------------------*/
#include "sf_gpio_hal.h"
#include "sf_adc_hal.h"
#include "sf_charger_measurements_hal.h"
#include "sf_charger_external_adc_hal.h"
#include "sf_timer_hal.h"

/* General defines ------------------------------------------------------------------*/

/* General defines (pinout mapping) ------------------------------------------------------------------*/
/**
 * @brief channel mapping for the Iout
 */
#define IOUT_ADC_CHANNEL			SF_ADC_CHANNEL_4

/**
 * @brief channel mapping for the Iin
 */
#define IIN_ADC_CHANNEL				SF_ADC_CHANNEL_15

/**
 * @brief channel mapping for the Vout
 */
#define VOUT_ADC_CHANNEL			SF_ADC_CHANNEL_14

/**
 * @brief channel mapping for the Vbus
 */
#define VBUS_ADC_CHANNEL			SF_ADC_CHANNEL_12

/**
 * @brief channel mapping for the power PCB temp
 */
#define POWERPCBTEMP_ADC_CHANNEL	SF_ADC_CHANNEL_0

/**
 * @brief channel mapping for the battery voltage
 */
#define VBAT_ADC_CHANNEL			SF_ADC_CHANNEL_3

/**
 * @brief channel mapping for the 24 V
 */
#define SUPPLY24V_ADC_CHANNEL		SF_ADC_CHANNEL_10

/**
 * @brief channel mapping for the HW VERSION
 */
#define HWVER_ADC_CHANNEL			SF_ADC_CHANNEL_8

/**
 * @brief channel mapping for the HW VERSION
 */
#define CONTROLPCBTEMP_ADC_CHANNEL	SF_ADC_CHANNEL_9

/*
 * @brief structure that contains the information to scale the conversions
 * @note It has the values to scale linear conversions through gain and offset (the formula would be measurement = gain*(ADC_pin_voltage - offset))
 * or exponential NTCs. Depending on the value, one or the other will be used.
 */
typedef struct {
	float 		gain;				/*!<Gain of the measurements. This gain must be from the ADC pin voltage to the final units*/
	float		offset;				/*!<Offset of the measurements. This offset is applied at the ADC pin*/
	float		T1_coefficient;		/*!<Coeeficient that multiplies the temperature in a PT resistor */
	float		T2_coefficient;		/*!<Coeeficient that multiplies the square of the temperature in a PT resistor */
	float 		R0;					/*!<Reference resistance (resistance at T0). Used in the NTCs and the PTs	*/
	float 		R_pullup;			/*!<Resistance used to make the voltage divider. Used in the NTCs and the PTs */
	float 		V_supply;			/*!<Supply voltage used to make the voltage divider. Used in the NTCs and the PTs */
	float		T0;					/*!<Reference temperature	*/
	float 		Beta;				/*!<Beta constant of the NTC */

} analog_measurements_scaling_factors_t;

/* Private variables ---------------------------------------------------------*/
#ifdef ENABLE_REALTIME_DEBUG
	// Define the variables as global variables

	/**
	 * @brief Array containing the fast measurements channel voltage without the scaling
	 */
	float measurements_channel_voltage [TOTAL_MEASUREMENTS] = {};

	/**
	 * @brief Array containing the fast measurements
	 */
	float measurements_local [TOTAL_MEASUREMENTS] = {};

	/**
	 * @brief Array containing all the final measurements
	 */
	analog_measurements_scaling_factors_t scaling_factors[TOTAL_MEASUREMENTS] = {};

	/**
	 * @brief Variable containing the configuration of the ADC1
	 */
	adc_config_t adc1 = ADC_CONFIG_DEFAULTS;

	/**
	 * @brief Variable containing the configuration of the ADC2
	 */
	adc_config_t adc2 = ADC_CONFIG_DEFAULTS;

#else
	// Define the variables as static to ensure they are local

	/**
	 * @brief Array containing the fast measurements channel voltage without the scaling
	 */
	static float measurements_channel_voltage [TOTAL_MEASUREMENTS] = {};

	/**
	 * @brief Array containing the fast measurements
	 */
	static float measurements_local [TOTAL_MEASUREMENTS] = {};

	/**
	 * @brief Array containing all the final measurements
	 */
	static analog_measurements_scaling_factors_t scaling_factors[TOTAL_MEASUREMENTS] = {};

	/**
	 * @brief Variable containing the configuration of the ADC1
	 */
	static adc_config_t adc1 = ADC_CONFIG_DEFAULTS;

	/**
	 * @brief Variable containing the configuration of the ADC2
	 */
	static adc_config_t adc2 = ADC_CONFIG_DEFAULTS;
#endif

/* Private functions headers ------------------------------------------------------------------*/


float get_conversion_linear_scaling (float pin_voltage, analog_measurements_scaling_factors_t scaling);
float get_conversion_NTC_scaling (float pin_voltage, analog_measurements_scaling_factors_t scaling);
float get_HW_version (float pin_voltage);

/* Public functions ------------------------------------------------------------------*/

/**
 * @brief Initialize everything that is needed for the analog measurements
 * @return status @ref measurement_status_t
 */
measurement_status_t init_measurements (void){

	// CONFIGURE THE GAINS OF EVERY CONVERSION
	scaling_factors[OUT_CURRENT].gain 					= -30.0f/20.0f*1/0.1f;							// 30*20 because of the voltage divider of 20 k and 10 k + According to datasheet (typ value): 100 mV/A. It is negative because the current enters through the negative. TMCS1100A2QDRQ1 (https://www.ti.com/lit/ds/symlink/tmcs1100-q1.pdf?ts=1749191564440&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FTMCS1100-Q1)
	scaling_factors[OUT_CURRENT].offset 				= 2.5f * 20.0f / 30.0f; 						// Voltage for the 0 A considering the voltage divider.

	scaling_factors[IN_CURRENT].gain 					= -30.0f/20.0f*1/0.1f;							// 30*20 because of the voltage divider of 20 k and 10 k + According to datasheet (typ value): 100 mV/A. It is negative because the current enters through the negative. TMCS1100A2QDRQ1 (https://www.ti.com/lit/ds/symlink/tmcs1100-q1.pdf?ts=1749191564440&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FTMCS1100-Q1)
	scaling_factors[IN_CURRENT].offset 					= 2.5f * 20.0f / 30.0f; 						// Voltage for the 0 A considering the voltage divider.

	scaling_factors[OUT_VOLTAGE].gain 					= (470.0f*3.0f+10.0f)/10.0f;					// Voltage divider between 10 k (meas) and 3 470 k. Maximum measured voltage 468V.
	scaling_factors[OUT_VOLTAGE].offset 				= 0.0f; 										//

	scaling_factors[BUS_VOLTAGE].gain 					= (470.0f*3.0f+3.3f)/3.3f;	 					// Voltage divider between 3k3 (meas) and 3 470 k. Maximum measured voltage 1413 V.
	scaling_factors[BUS_VOLTAGE].offset 				= 0.0f; 										//

	scaling_factors[AC1_VOLTAGE].gain 					= (2000.0f+12.0f)/12.0f; 						// Voltage divider between 12k (meas) and 2 1 M. Maximum measured voltage 419 V (adc configured at 2.5V).
	scaling_factors[AC1_VOLTAGE].offset 				= 0.0f; 										//

	scaling_factors[AC2_VOLTAGE].gain 					= (2000.0f+12.0f)/12.0f; 						// Voltage divider between 12k (meas) and 2 1 M. Maximum measured voltage 419 V (adc configured at 2.5V).
	scaling_factors[AC2_VOLTAGE].offset 				= 0.0f; 										//

	scaling_factors[CAN2PE_VOLTAGE].gain 				= (2000.0f+12.0f)/12.0f; 						// Voltage divider between 12k (meas) and 2 1 M. Maximum measured voltage 419 V (adc configured at 2.5V).
	scaling_factors[CAN2PE_VOLTAGE].offset 				= 0.0f; 										//

	scaling_factors[BAT_VOLTAGE].gain 					= (470.0f*3.0f+3.3f)/3.3f/2.0f;					//Voltage divider between 3k3 (meas) and 3 470 k and then a multiplier by 2 because of the isolated differential apmplifier AMC3330. Maximum measured voltage 706.5 V.
	scaling_factors[BAT_VOLTAGE].offset 				= 0.0f; 										//

	scaling_factors[POWERPCB_TEMP].gain 				= -1.0f/0.01123f;								// According to datasheet (typ value): 11.23 mV/ºC. MAX6613MXK+T (https://www.analog.com/media/en/technical-documentation/data-sheets/MAX6613.pdf=)
	scaling_factors[POWERPCB_TEMP].offset 				= 1.8455f; 										// According to datasheet (typ value)

	scaling_factors[CONTROLPCB_TEMP].R0 				= 10000.0f;										//Datasheet of the NTC --> NTCG163JF103FT1 (https://product.tdk.com/system/files/dam/doc/product/sensor/ntc/chip-ntc-thermistor/catalog/tpd_commercial_ntc-thermistor_ntcg_en.pdf)
	scaling_factors[CONTROLPCB_TEMP].T0 				= 25.0f;										//Datasheet of the NTC --> NTCG163JF103FT1 (https://product.tdk.com/system/files/dam/doc/product/sensor/ntc/chip-ntc-thermistor/catalog/tpd_commercial_ntc-thermistor_ntcg_en.pdf)
	scaling_factors[CONTROLPCB_TEMP].Beta 				= 3453.0f; 										//Datasheet of the NTC --> NTCG163JF103FT1 (https://product.tdk.com/system/files/dam/doc/product/sensor/ntc/chip-ntc-thermistor/catalog/tpd_commercial_ntc-thermistor_ntcg_en.pdf)
	scaling_factors[CONTROLPCB_TEMP].R_pullup 			= 10000.0f; 									//Schematics and temperature calculation spreadsheet
	scaling_factors[CONTROLPCB_TEMP].V_supply 			= 3.3f; 										//Schematics and temperature calculation spreadsheet

	scaling_factors[SUPPLY_24V].gain 					= (24.0f+1.0f)/1.0f; 							//Voltage divider between 1 k (meas) and 24k. Maximum measured voltage 82.5V.
	scaling_factors[SUPPLY_24V].offset 					= 0.0f; 										//


	// Initialization of the internal timer to measure ns time
	init_ns_timer (NS_TIMER_PERIPHERAL_NUMBER);

	// INITIALICE THE PERIPHERALS
	adc1.channels = SF_ADC_CHANNEL_2;							// Not used for the moment
	adc1.peripheral_number = (uint8_t) 1;						// ADC 1
	adc1.v_adc_ref = 3.3f;										//
	init_adc(adc1);												// Initialization


	// Right now to make a quick configuration the ADC2 is synchronized with the TIM1 as the ADC1
	// PENDING TO DEFINE HOW TO DO THIS "LOW-SPEED" CONVERSIONS
	adc2.channels = SF_ADC_CHANNEL_2;							// Not used for the moment
	adc2.peripheral_number = (uint8_t) 2;						// ADC 2
	adc2.interrupts.interrupt_selection = ADC_NO_INTERRUPT;		// No interrupt for the moment
	adc2.interrupts.callback_function = NULL;					// No function pointer for the interrupt
	adc2.v_adc_ref = 3.3f;										//
	init_adc(adc2);												// Initialization

	// Initialization of the external ADC
	external_adc_init();

	// TODO PENDING TO EVALUATE ERROS AND SO
	return MEASUREMENT_OK;
}


/**
 * @brief Start the low_speed_conversions
 * @return status @ref measurement_status_t
 */
measurement_status_t start_low_speed_conversions (void){

	// The same function to start the ADC as it is trigger by SW, it will make a conversion
	init_adc(adc2);

	// TODO PENDING TO EVALUATE ERROS AND SO
	return MEASUREMENT_OK;
}

/**
 * @brief Function to get all the high-speed measurements
 * @param measurements array pointer where the measurements should be stored.
 * @Note the input should have at least 7 positions to store the high-speed measurements
 * @return status @ref measurement_status_t
 */
measurement_status_t get_high_speed_measurements (float measurements []){
	// Get the raw value from the ADC conversion
	get_adc_conversion(adc1.peripheral_number,
			ADC_CHANNEL_MASK(IOUT_ADC_CHANNEL) | ADC_CHANNEL_MASK(VBUS_ADC_CHANNEL) | ADC_CHANNEL_MASK(VOUT_ADC_CHANNEL) | ADC_CHANNEL_MASK(IIN_ADC_CHANNEL),
			&measurements_channel_voltage[0]);

	get_external_adc_conversions (&measurements_channel_voltage[AC1_VOLTAGE]);

	// Auxiliary variable to do the scaling of the measurements_local
	uint8_t i= 0;

	// For loop to scale the conversions from the voltage at the input of the ADC to the real value.
	// Store the information in a local variable and in the array specified array in the input.
	for (i=0; i < 7; i++){
		measurements[i] = get_conversion_linear_scaling(measurements_channel_voltage[i], scaling_factors[i]);
		measurements_local [i] = measurements[i] ;
	}

	// As the output voltage measurement is done between the Vbus- and the Vout-. What it does really measure is the differente between the references.
	// Therefore, the real output voltage measurement is Vbus - Vout (the measure value)
	measurements[OUT_VOLTAGE] = measurements[BUS_VOLTAGE] - measurements[OUT_VOLTAGE];
	measurements_local[OUT_VOLTAGE] = measurements[OUT_VOLTAGE];

	// TODO Error management
	return MEASUREMENT_OK;

}

/**
 * @brief Function to get all the internal ADC high-speed measurements
 * @param measurements array pointer where the measurements should be stored.
 * @Note the input should have at least 4 positions to store the high-speed measurements
 * @return status @ref measurement_status_t
 */
measurement_status_t get_internal_adc_high_speed_measurements (float measurements []){
	// Get the raw value from the ADC conversion
	get_adc_conversion(adc1.peripheral_number,
			ADC_CHANNEL_MASK(IOUT_ADC_CHANNEL) | ADC_CHANNEL_MASK(VBUS_ADC_CHANNEL) | ADC_CHANNEL_MASK(VOUT_ADC_CHANNEL) | ADC_CHANNEL_MASK(IIN_ADC_CHANNEL),
			&measurements_channel_voltage[0]);

	// Auxiliary variable to do the scaling of the measurements_local
	uint8_t i= 0;

	// For loop to scale the conversions from the voltage at the input of the ADC to the real value.
	// Store the information in a local variable and in the array specified array in the input.
	for (i=0; i < 4; i++){
		measurements[i] = get_conversion_linear_scaling(measurements_channel_voltage[i], scaling_factors[i]);
		measurements_local [i] = measurements[i] ;
	}

	// As the output voltage measurement is done between the Vbus- and the Vout-. What it does really measure is the differente between the references.
	// Therefore, the real output voltage measurement is Vbus - Vout (the measure value)
	measurements[OUT_VOLTAGE] = measurements[BUS_VOLTAGE] - measurements[OUT_VOLTAGE];
	measurements_local[OUT_VOLTAGE] = measurements[OUT_VOLTAGE];

	// TODO Error management
	return MEASUREMENT_OK;

}

/**
 * @brief Function to get only the input current conversion all the internal ADC high-speed measurements
 * @param measurements array pointer where the measurements should be stored.
 * @Note the input should have at least 4 positions to store the high-speed measurements
 * @return status @ref measurement_status_t
 */
float get_input_current (void){
	// Get the raw value from the ADC conversion
	get_adc_conversion(adc1.peripheral_number,
			ADC_CHANNEL_MASK(IIN_ADC_CHANNEL),
			&measurements_channel_voltage[IN_CURRENT]);

	measurements_local[IN_CURRENT] = get_conversion_linear_scaling(measurements_channel_voltage[IN_CURRENT], scaling_factors[IN_CURRENT]);

	return measurements_local[IN_CURRENT];

}


/**
 * @brief Function to get all the measurements
 * @param measurements array pointer where the measurements should be stored.
 * @Note the input should have at least TOTAL_MEASUREMENTS (@ref measurements_t) positions to store the all the measurements
 * @return status @ref measurement_status_t
 */
measurement_status_t get_all_measurements (float measurements []){

	// Get first the high speed measurements and then obtain the low speed
//	get_high_speed_measurements (measurements);

	// Assumed that the high speed measurements are already done as they are suppose to be done constantly
	uint8_t i = 0;
	for (i = 0; i < POWERPCB_TEMP; i++){
		measurements[i] = measurements_local[i];
	}

	// Get the internal measurements of the MCU
	get_adc_internal_measurements (&measurements[MCU_TEMP]);

	//LOW SPEED MEASUREMENTS

	// Get the raw value from the ADC conversion
	get_adc_conversion(adc2.peripheral_number,
			ADC_CHANNEL_MASK(POWERPCBTEMP_ADC_CHANNEL) | ADC_CHANNEL_MASK(VBAT_ADC_CHANNEL) | ADC_CHANNEL_MASK(SUPPLY24V_ADC_CHANNEL) | ADC_CHANNEL_MASK(HWVER_ADC_CHANNEL) | ADC_CHANNEL_MASK(CONTROLPCBTEMP_ADC_CHANNEL),
			&measurements_channel_voltage[POWERPCB_TEMP]);


	// Auxiliary variable to do the scaling of the measurements_local
	i= 0;
	measurements_t index_aux;

	// For loop to scale the conversions from the voltage at the input of the ADC to the real value.
	// Store the information in a local variable and in the array specified array in the input.
	for (i = 0; i < 5; i++){

		index_aux = POWERPCB_TEMP+i;

		// If the measurement to scale it the PCB temp, it is needed to call other scaling function as it is not linear
		if (index_aux == CONTROLPCB_TEMP){
			measurements[index_aux] = get_conversion_NTC_scaling (measurements_channel_voltage[index_aux], scaling_factors[index_aux]);
			measurements_local [index_aux] = measurements[index_aux];

		// If the measurement to scale it the HW VERSION, it is needed to call other scaling function as it is not linear
		} else if (index_aux == HW_VERSION) {
			measurements[index_aux] = get_HW_version (measurements_channel_voltage [index_aux]);
			measurements_local [index_aux] = measurements[index_aux];

		// All the other functions are scaled with the linear function
		} else {
			measurements[index_aux] = get_conversion_linear_scaling(measurements_channel_voltage[index_aux], scaling_factors[index_aux]);
			measurements_local [index_aux] = measurements[index_aux];
		}
	}

	// TODO Error management
	return MEASUREMENT_OK;

}


/**
 * @brief Function to define which is the callback function after doing the fast measurements
 * @return status @ref measurement_status_t
 */
measurement_status_t set_measurements_callback_function (void (*callback_function)(void)){

	if (callback_function != NULL){
		adc1.interrupts.callback_function = callback_function;
		adc1.interrupts.interrupt_selection = ADC_IT_ENDOFCONVERSION_DMA;

		// TODO
		return MEASUREMENT_OK;
	} else {
		return MASUREMENT_ERROR;
	}
}

/* Private functions ------------------------------------------------------------------*/

/**
 * @brief Calculates the HW version of the PCB
 * @param pointer where the HW version must be stored
 * @return status @ref measurement_status_t
 */
float get_HW_version (float pin_voltage) {

	// Assumed that each 200 mV is a new version, therefore, there can be up to 16 versions

	// Function from chatGPT. The HW version is:
    //   - 0 for voltage reading of [0.0 .. 0.2)
    //   - 1 for voltage reading of [0.2 .. 0.4)
    //   - 2 for voltage reading of [0.4 .. 0.6)
    //   ...
    //   - 15 for voltage reading of [3.0 .. 3.2)
    //   - 16 for voltage reading of [3.2 .. 3.4)
	// floorf(x) round down x to the nearest integer, as a float.
	uint8_t uint_HW_version = (uint8_t) floorf(pin_voltage * 5.0f);

	// As the expected value is float:
	return (float) uint_HW_version;

	// PENDING TO EVALUATE WRONG MEASUREMENTS
	return MEASUREMENT_OK;
}

/**
 * @brief common function scale any linear conversion
 * @param pin_voltage is the voltage measured in the ADC pin
 * @param scaling are the parameters needed to scale the pin voltage to the final measurement
 * @return the measurement
 */
float get_conversion_linear_scaling (float pin_voltage, analog_measurements_scaling_factors_t scaling){

	return (float) (pin_voltage - scaling.offset)*scaling.gain;
}

/**
 * @brief common function to calculate the temperature of a NTC sensor
 * @param pin_voltage is the voltage measured in the ADC pin
 * @param scaling are the parameters needed to scale the pin voltage to the final measurement
 * @return the measurement
 */
float get_conversion_NTC_scaling (float pin_voltage, analog_measurements_scaling_factors_t scaling) {

	// This is the denominator in the next operation. It is calculated beforehand to avoid possible divisions by 0.
	float denom = (scaling.V_supply- pin_voltage);


	if (denom <= 0) {
		// If the denominator is 0 or lower, it means that there is some error. So, a non-logical result is returned
		return 1000.0f;

	} else {
		// Get the resistance of the sensor
		float R_sensor = scaling.R_pullup*pin_voltage / denom;

		// Calculate the temperature according to the scaling factors and the function of the NTC formula (used this as a reference: https://www.giangrandi.org/electronics/ntc/ntc.shtml)
		return (float) 1/((log(R_sensor/scaling.R0)/scaling.Beta)+1/(scaling.T0 + 273.15f)) - 273.15f;
	}
}
