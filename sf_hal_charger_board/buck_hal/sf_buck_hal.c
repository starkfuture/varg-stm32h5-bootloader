/**
 * @file sf_buck_hal.c
 * @brief Hardware Abstraction Layer to manage the Buck converter of the charger
 * @note Check that the project build configuration includes this path (Properties > C/C++ Build > Settings > Tool Settings > MCU/MPU GCC Compiler > Includes).
 * 			If not, add ../sf_hal_charger_board/buck_hal. Also check that this folder is not excluded from Build (Right click on scheduler folder > Properties > C/C++ Build > Exclude Resource from Build).
 * @author Lucas Bouzon Pousa
 * @date 05/06/2024
 */

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include "sf_buck_hal.h"
#include "sf_gpio_hal.h"
#include "sf_pwm_hal.h"

/* General defines (pinout mapping) ------------------------------------------------------------------*/
/**
 * @brief Definition of the channel of the PWM of the Buck
*/
#define BUCK_PWM_CHANNEL	     	PWM_CHANNEL_4
/**
 * @brief Definition of the peripheral of the PWM of the Buck
*/
#define BUCK_PWM_PERIPHERAL	     	(uint8_t) 4
/**
 * @brief FLT GPIO of the PWM1
*/
#define BUCK_PWM_FAULT	     		gpio_A12



/**
 * @brief Number of branches the buck has.
*/
#define BUCK_BRANCHES	     		(uint8_t) 1

/**
 * @brief Initial PFC frequency in kHz
*/
#define BUCK_INIT_FREQ	     		(uint16_t) 100

//Enable these 2 defines to declare the variable as global for Real Time Debug
// NOTE: Be careful with global definition because it may be collisions with other
//	global variables
// NOTE2: This define may be declared in Properties > C/C++ Build > Settings > MCU GCC Compiler > Symbol
/* Private variables ---------------------------------------------------------*/
#ifdef ENABLE_REALTIME_DEBUG
	// Define the variables as global variable

	/*
	 *  @brief Variable to indicate if the PFC is ON or OFF
	 */
	bool buck_on = false;

	/*
	 *  @brief Variable containing the pwm configuration of the PWM1
	 */
	pwm_config_t buck_pwm_config = PWM_CONFIG_DEFAULTS;

	/*
	 *  @brief Variable containing the general information of the buck
	 */
	buck_info_t buck_local_information = {
			.available_power = 1.0f,
			.status = BUCK_OK,
			.wroking_branches = (uint8_t) 1,
	};


#else
	// Define the variables as static to ensure they are local

	/*
	 *  @brief Variable to indicate if the PFC is ON or OFF
	 */
	static bool buck_on = false;

	/*
	 *  @brief Variable containing the pwm configuration of the PWM1
	 */
	static pwm_config_t buck_pwm_config = PWM_CONFIG_DEFAULTS;

	/*
	 *  @brief Variable containing the general information of the buck
	 */
	static buck_info_t buck_local_information = {
			.available_power = 1.0f,
			.status = BUCK_OK,
			.wroking_branches = (uint8_t) 1,
	};

#endif


/* Public Functions ------------------------------------------------------------------*/

/**
 * @brief Initialize and start the BUCK PWMs
 */
void init_buck (float buck_switching_freq_khz){

	// Variable to indicate that the PFC is ON
	buck_on = true;

	// Initialization of PWM1
	buck_pwm_config.channels = BUCK_PWM_CHANNEL;
	buck_pwm_config.peripheral_number  = BUCK_PWM_PERIPHERAL;
	buck_pwm_config.frequency_kHz = buck_switching_freq_khz;
	buck_pwm_config.interrupts.ADC_tigger_interrupt_frequency_kHz = 20.0f;
//	buck_pwm_config.interrupts.interrupt_selection = PWM_IT_PERIOD;
	init_pwm(buck_pwm_config);

}

/**
 * @brief Function to define which is the callback function after each cyclo of the buck PWM
 */
void set_buck_callback_function (void (*callback_function)(void)){

	if (callback_function != NULL){
		buck_pwm_config.interrupts.callback_function = callback_function;
		buck_pwm_config.interrupts.interrupt_selection = PWM_IT_PERIOD;
	} else {
		// DO NOTHING
	}
}

/**
 * @brief Stop the BUCK PWMs
 */
void stop_buck (void){

	// Force the duty cycle to 0% (no switching)
	set_buck_duty (0.0f);

	// Variable to indicate that the buck is OFF
	buck_on = false;

	// Disable all the outputs
	disable_pwm_output(BUCK_PWM_PERIPHERAL,BUCK_PWM_CHANNEL);

}

/**
 * @brief After stopping the PWM, the peripheral is still working, but to do an enable again, just init again the peripheral with the stored configuration
 */
void enable_buck (void){
	init_pwm(buck_pwm_config);

	// Variable to indicate that the buck is ON
	buck_on = true;
}

/**
 * @brief Set the duty cycle of the BUCK
 * @param duty_pu is the duty to be applied in the BUCK. This duty cycle is in pu (0 -> 0 %, 1 -> 100%)
 */
void set_buck_duty (float duty_pu){

	//if the pfc is not ON, do not modify the duty.
	if (buck_on == true){
		set_pwm_duty (BUCK_PWM_PERIPHERAL, BUCK_PWM_CHANNEL, duty_pu);
	} else {}

}

/**
 * @brief Set the switching frequency of the BUCK
 * @param frequency_khz is the switching frequency to be applied in the BUCK. This frequency is in kHz.
 */
void set_buck_frequency (float frequency_khz){

	buck_pwm_config.frequency_kHz = frequency_khz;
	set_pwm_frequency (buck_pwm_config);
}


buck_info_t get_buck_info (void){

	// To know the working branches, read all the faults and sum the number of not faulty branches (as the drivers gives a 0 when there is a problem)
	buck_local_information.wroking_branches = (gpio_get(BUCK_PWM_FAULT));


	//If there are no working branches, the status is a NOK
	if (buck_local_information.wroking_branches == 0){
		buck_local_information.status = BUCK_NOK;

	//If not all the branches are working, but at least has one working branch, the status will be warning.
	} else if (buck_local_information.wroking_branches < BUCK_BRANCHES ){
		buck_local_information.status = BUCK_WARNING;

	//If all the branches are working properly, the status is OK.
	} else {
		buck_local_information.status = BUCK_OK;
	}

	// Calculate the available power according to the number of branches and if the buck is on or off.
	buck_local_information.available_power = (buck_local_information.wroking_branches) * (float) buck_on;

	return buck_local_information;
}
