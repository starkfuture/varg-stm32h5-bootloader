/**
 * @file sf_pfc_hal.c
 * @brief Hardware Abstraction Layer to manage the PFC of the charger
 * @note Check that the project build configuration includes this path (Properties > C/C++ Build > Settings > Tool Settings > MCU/MPU GCC Compiler > Includes).
 * 			If not, add ../sf_hal_charger_board/pfc_hal. Also check that this folder is not excluded from Build (Right click on scheduler folder > Properties > C/C++ Build > Exclude Resource from Build).
 * @author Lucas Bouzon Pousa
 * @date 28/05/2024
 */

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include "sf_pfc_hal.h"
#include "sf_gpio_hal.h"
#include "sf_pwm_hal.h"

/* General defines (pinout mapping) ------------------------------------------------------------------*/

/**
 * @brief Definition of the channel of the PWM1 of the PFC
*/
#define PFC_PWM1_CHANNEL	     		PWM_CHANNEL_2
/**
 * @brief Definition of the channel of the PWM timer used to synchronize to other PWMs
*/
#define PFC_PWM1_SYNC_CHANNEL	     	PWM_CHANNEL_1
/**
 * @brief Definition of the peripheral of the PWM1 of the PFC
*/
#define PFC_PWM1_PERIPHERAL	     		(uint8_t) 3
/**
 * @brief FLT GPIO of the PWM1
*/
#define PFC_PWM1_FAULT	     			gpio_E3

/**
 * @brief Definition of the channel of the PWM2 of the PFC
*/
#define PFC_PWM2_CHANNEL	     		PWM_CHANNEL_4
/**
 * @brief Definition of the channel of the PWM timer used to synchronize to other PWMs
*/
#define PFC_PWM2_SYNC_CHANNEL	     	PWM_CHANNEL_1
/**
 * @brief Definition of the peripheral of the PWM2 of the PFC
*/
#define PFC_PWM2_PERIPHERAL	     		(uint8_t) 8
/**
 * @brief FLT GPIO of the PWM2
*/
#define PFC_PWM2_FAULT	     			gpio_D0

/**
 * @brief Definition of the channel of the PWM3 of the PFC
*/
#define PFC_PWM3_CHANNEL	     		PWM_CHANNEL_4
/**
 * @brief Definition of the channel of the PWM timer used to synchronize to other PWMs
*/
#define PFC_PWM3_SYNC_CHANNEL	     	PWM_CHANNEL_1
/**
 * @brief Definition of the peripheral of the PWM3 of the PFC
*/
#define PFC_PWM3_PERIPHERAL	     		(uint8_t) 1
/**
 * @brief FLT GPIO of the PWM3
*/
#define PFC_PWM3_FAULT	     			gpio_C11

/**
 * @brief Number of branches the PFC has.
*/
#define PFC_BRANCHES	     			(uint8_t) 3

/**
 * @brief Initial PFC frequency in kHz
*/
#define PFC_INIT_FREQ	     			(uint16_t) 100

/**
 * @brief Frequency of the interrupt to execute the controls
*/
#define CONTROL_IT_FREQ	     			(uint16_t) 20


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
	bool pfc_on = false;

	/*
	 *  @brief Variable containing the general information of the PFC
	 */
	pfc_info_t pfc_local_information = {
			.available_power = 1.0f,
			.status = PFC_OK,
			.wroking_branches = (uint8_t) 3,
	};

	/*
	 *  @brief Variable containing the pwm configuration of the PWM1
	 */
	pwm_config_t pfc_pwm1_config = PWM_CONFIG_DEFAULTS;

	/*
	 *  @brief Variable containing the pwm configuration of the PWM2
	 */
	pwm_config_t pfc_pwm2_config = PWM_CONFIG_DEFAULTS;

	/*
	 *  @brief Variable containing the pwm configuration of the PWM3
	 */
	pwm_config_t pfc_pwm3_config = PWM_CONFIG_DEFAULTS;

	/*
	 *  @brief Dummy pwm configuration just to generate an interrupt
	 */
	pwm_config_t pfc_pwm_int_config = PWM_CONFIG_DEFAULTS;


#else
	// Define the variables as static to ensure they are local

	/*
	 *  @brief Variable to indicate if the PFC is ON or OFF
	 */
	static bool pfc_on = false;

	/*
	 *  @brief Variable containing the general information of the PFC
	 */
	static pfc_info_t pfc_local_information = {
			.available_power = 1.0f,
			.status = PFC_OK,
			.wroking_branches = (uint8_t) 3,
	};

	/*
	 *  @brief Variable containing the pwm configuration of the PWM1
	 */
	static pwm_config_t pfc_pwm1_config = PWM_CONFIG_DEFAULTS;

	/*
	 *  @brief Variable containing the pwm configuration of the PWM2
	 */
	static pwm_config_t pfc_pwm2_config = PWM_CONFIG_DEFAULTS;

	/*
	 *  @brief Variable containing the pwm configuration of the PWM3
	 */
	static pwm_config_t pfc_pwm3_config = PWM_CONFIG_DEFAULTS;

#endif


/* Public Functions ------------------------------------------------------------------*/

/**
 * @brief Initialize and start the PFC PWMs
 */
void init_pfc (float pfc_switching_freq_khz, float pfc_current_control_it_freq_khz, float pfc_voltage_control_it_freq_khz){

	// Variable to indicate that the PFC is ON
	pfc_on = true;

	// Initialization of PWM1
	pfc_pwm1_config.channels = PFC_PWM1_CHANNEL;
	pfc_pwm1_config.peripheral_number  = PFC_PWM1_PERIPHERAL;
	pfc_pwm1_config.frequency_kHz = pfc_switching_freq_khz;
	pfc_pwm1_config.sync_channels = PFC_PWM1_SYNC_CHANNEL;
	pfc_pwm1_config.interrupts.ADC_tigger_interrupt_frequency_kHz = pfc_current_control_it_freq_khz;			// The ADC trigger will be synchronized with this PWM
	init_pwm(pfc_pwm1_config);

	// Initialization of PWM2
	pfc_pwm2_config.channels = PFC_PWM2_CHANNEL;
	pfc_pwm2_config.peripheral_number  = PFC_PWM2_PERIPHERAL;
	pfc_pwm2_config.frequency_kHz = pfc_switching_freq_khz;
	pfc_pwm2_config.sync_channels = PFC_PWM2_SYNC_CHANNEL;
	pfc_pwm2_config.interrupts.ADC_tigger_interrupt_frequency_kHz = pfc_current_control_it_freq_khz;			// The ADC trigger will be synchronized with this PWM
	init_pwm(pfc_pwm2_config);

	// Initialization of PWM3
	pfc_pwm3_config.channels = PFC_PWM3_CHANNEL;
	pfc_pwm3_config.peripheral_number  = PFC_PWM3_PERIPHERAL;
	pfc_pwm3_config.frequency_kHz = pfc_switching_freq_khz;
	pfc_pwm3_config.sync_channels = PFC_PWM3_SYNC_CHANNEL;
	pfc_pwm3_config.interrupts.ADC_tigger_interrupt_frequency_kHz = pfc_current_control_it_freq_khz;			// The ADC trigger will be synchronized with this PWM
	init_pwm(pfc_pwm3_config);

	pfc_pwm_int_config.channels = PWM_CHANNEL_1;
	pfc_pwm_int_config.peripheral_number  = 17;
	pfc_pwm_int_config.frequency_kHz = pfc_voltage_control_it_freq_khz;
	pfc_pwm_int_config.interrupts.ADC_tigger_interrupt_frequency_kHz = pfc_voltage_control_it_freq_khz;			// The ADC trigger will be synchronized with this PWM
	init_pwm(pfc_pwm_int_config);

	// Initialization of the phase-shifts between PWMs
	set_pwm_phase_shift (pfc_pwm3_config, pfc_pwm2_config, 0.333f);
	set_pwm_phase_shift (pfc_pwm2_config, pfc_pwm1_config, 0.333f);

}

void set_pfc_callback_function (void (*callback_function)(void)){

	if (callback_function != NULL){
		pfc_pwm_int_config.interrupts.callback_function = callback_function;
		pfc_pwm_int_config.interrupts.interrupt_selection = PWM_IT_PERIOD;
	} else {
		// DO NOTHING
	}
}

/**
 * @brief Stop the PFC PWMs
 */
void stop_pfc (void){

	// Force the duty cycle to 0% (no switching)
	set_pfc_duty (0.0f);

	// Variable to indicate that the PFC is OFF
	pfc_on = false;

	// Disable all the outputs
	disable_pwm_output(PFC_PWM1_PERIPHERAL,PFC_PWM1_CHANNEL);
	disable_pwm_output(PFC_PWM2_PERIPHERAL,PFC_PWM2_CHANNEL);
	disable_pwm_output(PFC_PWM3_PERIPHERAL,PFC_PWM3_CHANNEL);

	// Not a full-stop of the PWMs because the timer is the trigger of the ADC.

	// As the PFC is stopped, the available power will be 0
	pfc_local_information.available_power = 0.0f;

}

/**
 * @brief After stopping the PWM, the peripheral is still working, but to do an enable again, just init again the peripheral with the stored configuration
 */
void enable_pfc (void){

	init_pwm(pfc_pwm1_config);
	init_pwm(pfc_pwm2_config);
	init_pwm(pfc_pwm3_config);

	// Variable to indicate that the PFC is ON
	pfc_on = true;
}

/**
 * @brief Set the duty cycle of the PFC
 * @param duty_pu is the duty to be applied in the PFC. This duty cycle is in pu (0 -> 0 %, 1 -> 100%)
 * @Note all the branches will have the same duty cycle
 */
void set_pfc_duty (float duty_pu){

	//if the pfc is not ON, do not modify the duty.
	if (pfc_on == true){
		set_pwm_duty (PFC_PWM1_PERIPHERAL, PFC_PWM1_CHANNEL, duty_pu);
		set_pwm_duty (PFC_PWM2_PERIPHERAL, PFC_PWM2_CHANNEL, duty_pu);
		set_pwm_duty (PFC_PWM3_PERIPHERAL, PFC_PWM3_CHANNEL, duty_pu);
	} else {

	}

}

/**
 * @brief Set the switching frequency of the PFC
 * @param frequency_khz is the switching frequency to be applied in the PFC. This frequency is in kHz.
 * @Note all the branches will have the same switching frequency keeping the same phase-shift as before.
 */
void set_pfc_frequency (float frequency_khz){

	pfc_pwm1_config.frequency_kHz = frequency_khz;
	pfc_pwm2_config.frequency_kHz = frequency_khz;
	pfc_pwm3_config.frequency_kHz = frequency_khz;
	set_pwm_frequency (pfc_pwm1_config);
	set_pwm_frequency (pfc_pwm2_config);
	set_pwm_frequency (pfc_pwm3_config);
	set_pwm_phase_shift (pfc_pwm3_config, pfc_pwm2_config, 0.333f);
	set_pwm_phase_shift (pfc_pwm2_config, pfc_pwm1_config, 0.333f);

}

float get_pfc_available_power (void){

	return 0.0f;
}

pfc_info_t get_pfc_info (void){

	/* TO DEBUG
	volatile uint8_t status_branch1 = gpio_get(PFC_PWM1_FAULT);
	volatile uint8_t status_branch2 = gpio_get(PFC_PWM2_FAULT);
	volatile uint8_t status_branch3 = gpio_get(PFC_PWM3_FAULT);
	*/

	// To know the working branches, read all the faults and sum the number of not faulty branches (as the drivers gives a 0 when there is a problem)
	pfc_local_information.wroking_branches = (gpio_get(PFC_PWM1_FAULT)) + (gpio_get(PFC_PWM2_FAULT)) + (gpio_get(PFC_PWM3_FAULT));


	//If there are no working branches, the status is a NOK
	if (pfc_local_information.wroking_branches == 0){
		pfc_local_information.status = PFC_NOK;

	//If not all the branches are working, but at least has one working branch, the status will be warning.
	} else if (pfc_local_information.wroking_branches < PFC_BRANCHES ){
		pfc_local_information.status = PFC_WARNING;

	//If all the branches are working properly, the status is OK.
	} else {
		pfc_local_information.status = PFC_OK;
	}

	// Calculate the available power according to the number of branches and if the pfc is on or off.
	pfc_local_information.available_power = (pfc_local_information.wroking_branches*0.333333333f) * (float) pfc_on;

	return pfc_local_information;
}



