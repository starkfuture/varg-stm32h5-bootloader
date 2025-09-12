/**
 * @file sf_charger_fan_hal.h
 * @brief fan abstraction layer interface
 * @note Check that the project build configuration includes this path (Properties > C/C++ Build > Settings > Tool Settings > MCU/MPU GCC Compiler > Includes).
 * 			If not, add ../sf_hal_charger_board/fans_hal. Also check that this folder is not excluded from Build (Right click on scheduler folder > Properties > C/C++ Build > Exclude Resource from Build).
 * @author Lucas Bouzon Pousa
 * @date 13/06/2024
 */

/* Includes ------------------------------------------------------------------*/

/* General defines ------------------------------------------------------------------*/


/* Public Functions ------------------------------------------------------------------*/
void turn_on_fan (void);
void turn_off_fan (void);
void set_fan_speed (float speed);
float read_fan_speed (void);

