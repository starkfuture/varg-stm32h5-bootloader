/**
 * @file sf_charger_fan_hal.h
 * @brief fan abstraction layer interface
 * @note Check that the project build configuration includes this path (Properties > C/C++ Build > Settings > Tool Settings > MCU/MPU GCC Compiler > Includes).
 * 			If not, add ../sf_hal_charger_board/fans_hal. Also check that this folder is not excluded from Build (Right click on scheduler folder > Properties > C/C++ Build > Exclude Resource from Build).
 * @author Lucas Bouzon Pousa
 * @date 13/06/2024
 */
 
/* Includes ------------------------------------------------------------------*/
#include "sf_charger_fan_hal.h"
#include "sf_gpio_hal.h"
#include "sf_pwm_hal.h"

/* General defines (pinout mapping) ------------------------------------------------------------------*/
/**
 * @brief GPIO to turn on/off the supply
*/
#define FAN_SUPPLY_PIN	     	gpio_B6

/**
 * @brief Definition of the channel of the PWM1 of the PFC
*/
#define FAN_PWM_CHANNEL	     	PWM_CHANNEL_1
/**
 * @brief Definition of the peripheral of the PWM1 of the PFC
*/
#define FAN_PWM_PERIPHERAL	    (uint8_t) 16


/* General defines  ------------------------------------------------------------------*/
/**
 * @brief PWM frequency used to control the speed
*/
#define FAN_PWM_FREQ_KHZ	20.0f

/**
 * @brief Minimum allowed speed for the fan
*/
#define FAN_MIN_SPEED		 0.2f

/**
 * @brief Maximum allowed speed for the fan
*/
#define FAN_MAX_SPEED		 1.0f

//Enable these 2 defines to declare the variable as global for Real Time Debug
// NOTE: Be careful with global definition because it may be collisions with other
//	global variables
// NOTE2: This define may be declared in Properties > C/C++ Build > Settings > MCU GCC Compiler > Symbol
/* Private variables ---------------------------------------------------------*/
#ifdef ENABLE_REALTIME_DEBUG
	// Define the variables as global variable
	/*
	 *  @brief Variable containing the pwm configuration of the fans
	 */
	pwm_config_t fan_pwm_config = PWM_CONFIG_DEFAULTS;

#else
	// Define the variables as static to ensure they are local

	/*
	 *  @brief Variable containing the pwm configuration of the fans
	 */
	static pwm_config_t fan_pwm_config = PWM_CONFIG_DEFAULTS;

#endif

/* Public Functions ------------------------------------------------------------------*/

/**
 * @brief Initializes and turns on the fan.
 *
 * This function initializes the PWM with the defined configuration
 * for the fan and sets its duty cycle to the minimum speed.
 * It also enables the power supply to the fan via GPIO.
 */
void turn_on_fan (void){

	// Initialize the fan PWM
	fan_pwm_config.channels = FAN_PWM_CHANNEL;
	fan_pwm_config.frequency_kHz = FAN_PWM_FREQ_KHZ;
	fan_pwm_config.peripheral_number = FAN_PWM_PERIPHERAL;
	init_pwm(fan_pwm_config);

	// Set the minimum speed
	set_pwm_duty(FAN_PWM_PERIPHERAL,FAN_PWM_CHANNEL,FAN_MIN_SPEED);

	//Enable the power supply of the fan
	gpio_set(FAN_SUPPLY_PIN);

}

/**
 * @brief Turns off the fan.
 *
 * This function sets the duty cycle to the minimum speed, disables
 * the fan power supply, and disables the PWM output.
 */
void turn_off_fan (void){

	// Set the minimum speed
	set_pwm_duty(FAN_PWM_PERIPHERAL,FAN_PWM_CHANNEL,FAN_MIN_SPEED);

	//Disable the power supply of the fan
	gpio_clear(FAN_SUPPLY_PIN);

	// Disable the pwm
	disable_pwm_output(FAN_PWM_PERIPHERAL,FAN_PWM_CHANNEL);

}

/**
 * @brief Sets the fan speed.
 *
 * This function sets the duty cycle of the fan's PWM signal
 * according to the input speed. It clamps the speed between
 * FAN_MIN_SPEED and FAN_MAX_SPEED.
 *
 * @param speed Desired fan speed as a float value (from 0 to 1).
 *              It is automatically clamped to valid limits.
 */
void set_fan_speed (float speed){

	float fan_pwm_duty = speed; 		//Auxiliary variable that can be modify if the input is outside the limits.

	//Check that the speed is between the limits
	if (fan_pwm_duty < FAN_MIN_SPEED){
		fan_pwm_duty = FAN_MIN_SPEED;
	} else if (fan_pwm_duty > FAN_MAX_SPEED){
		fan_pwm_duty = FAN_MAX_SPEED;
	} else {
		//DO NOTHING
	}

	// Set the speed
	set_pwm_duty(FAN_PWM_PERIPHERAL,FAN_PWM_CHANNEL,fan_pwm_duty);

}


float read_fan_speed (void) {

	//TODO
	return 0.0f;
}
