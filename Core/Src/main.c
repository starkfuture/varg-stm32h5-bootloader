/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "fdcan.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sf_can_hal.h"
#include "bootloader.h"
#include "sf_bootloader_hal.h"
#include "sf_flash_hal.h"
#include "sf_crc_hal.h"
#include "mem.h"
#include "can_message_handler.h"
#include "sf_timer_hal.h"
#include "sf_charger_led_hal.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STARK_CHARGER 4
#define STARK_INVERTER 5

#define BOARD STARK_CHARGER

#define BOOTLOADER_BTEA_BUFFER_SIZE         0x2000U

#ifndef HW_VERSION
//#error "HW_VERSION needs to be defined"
#define HW_VERSION 0
#endif

#define BOOTLOADER_VERSION                  0x01

#if BOARD == STARK_CHARGER

#define BOOTLOADER_ECU_CODE_ID              4U
const public_key_t public_key = {
		.key = {
				0xc4, 0x05, 0xad, 0xcf, 0xbe, 0x78, 0x79, 0x58, 0x6a, 0xbe, 0x6f, 0x5a, 0x20, 0x27, 0x3f, 0xc9,
				0x4e, 0xf0, 0x7c, 0xc5, 0x7b, 0xbe, 0xcc, 0x43, 0xe9, 0xa0, 0xc3, 0x77, 0x70, 0x0d, 0x69, 0x29,
				0xb6, 0x9d, 0xae, 0xf1, 0x62, 0x3b, 0x5e, 0x90, 0x32, 0x9b, 0x2b, 0x82, 0x71, 0xd4, 0x55, 0x4e,
				0x19, 0x2d, 0xfe, 0x31, 0x0c, 0x1d, 0x7d, 0x11, 0x80, 0xf6, 0x0e, 0x25, 0xaa, 0x2e, 0x82, 0xc7
		}
};
const btea_key_t btea_key = {
    .key = {
        0x474657E4, 0x11AC1600, 0x4577F6F4, 0x56F4387D
    }
};

#elif BOARD == STARK_INVERTER
#define BOOTLOADER_ECU_CODE_ID              5U

const public_key_t public_key = {
		.key = {
				0x6e, 0xe7, 0x68, 0x2d, 0x5e, 0x20, 0xea, 0x16, 0x31, 0x21, 0x17, 0x32, 0x0b, 0x36, 0x70, 0x04,
				0x95, 0x7b, 0x8e, 0xe0, 0x9d, 0x39, 0x02, 0x3c, 0x29, 0xef, 0xb9, 0x3e, 0x1e, 0x45, 0x9c, 0x34,
				0x81, 0x4a, 0x20, 0x32, 0x2e, 0x03, 0xd1, 0x33, 0x32, 0xfd, 0xad, 0x32, 0xe8, 0xaa, 0x36, 0xb3,
				0x02, 0xf6, 0x70, 0xa9, 0x46, 0x7d, 0x04, 0x91, 0x5f, 0x65, 0x9e, 0x4f, 0x25, 0xd6, 0x9d, 0x94
		}
};
const btea_key_t btea_key = {
    .key = {
        0x8296938A, 0x69226073, 0x63D1A473, 0xC8904B38
    }
}

#else
#error "BOARD needs to be defined"
#endif

#define BOOTLOADER_LED_TIME_TOGGLE		1000

#define BOOT_MAGIC 						(0xDEADBEEFu)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t btea_buffer[BOOTLOADER_BTEA_BUFFER_SIZE];

const bootloader_config_t bootloader_config = {
    .device_id = BOOTLOADER_ECU_CODE_ID,
    .btea_buffer = btea_buffer,
    .padding = {
        .num_bytes = MEM_FLASH_WRITE_ALIGNMENT,
        .padding_byte = 0xFF
    },
    .btea_chunk_size = BOOTLOADER_BTEA_BUFFER_SIZE,
    .btea_key = &btea_key,
    .public_key = &public_key,
    .jump_to_app_func = sf_bootloader_hal_jump_to_app,
    .crc32_func = sf_bootloader_hal_crc32_func,
    .log_func = sf_bootloader_hal_log_func,
    .magic = BOOTLOADER_MAGIC,
    .mem_read_func = mem_read,
    .mem_write_func = mem_write,
    .mem_copy_func = mem_copy,
    .send_msg_func = sf_bootloader_hal_send_bootloader_message,
    .max_copy_retries = 3,
    .jump_delay = 200,
};

static uint32_t led_timer = 0;

__attribute__((section(".shared_ram"), used)) volatile uint32_t shared_variable;

typedef struct {
    uint32_t magic_number;  // Magic number to identify the hardware
    uint16_t board_version; // Hardware version (combination of BOARD and HW_VERSION)
    uint16_t crc;           // CRC of the above data
} hardware_info_t;

__attribute__((section(".shared_ram"), used)) volatile hardware_info_t hardware_info;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static inline bool is_software_reset(void)
{
    /* Read RCC->RSR SFTRSTF bit */
    return ((RCC->RSR & RCC_RSR_SFTRSTF) != 0u);
}

static inline void clear_reset_flags(void)
{
    /* Set RMVF bit to clear all reset flags */
    RCC->RSR |= RCC_RSR_RMVF;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral */
  LL_PWR_DisableUCPDDeadBattery();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FDCAN1_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */

  hardware_info.magic_number = 0xACABACAB;
  hardware_info.board_version = (BOARD | (HW_VERSION << 8));

  // Calculate the CRC for the fields `magic_number` and `board_version`
  uint32_t temp_data[2];
  temp_data[0] = hardware_info.magic_number;
  temp_data[1] = hardware_info.board_version;

  hardware_info.crc = sf_crc_compute_crc16_deadbeef( temp_data, sizeof(temp_data));

  /* Write your local variable definition here */
  bool stay_in_bootloader = false;

  if (is_software_reset() && (shared_variable == BOOT_MAGIC)) {
	  stay_in_bootloader = true;
  }

  /* Clear flags so the next reset cause is detectable */
  clear_reset_flags();

  shared_variable = 0u;

  mem_init();

  can_message_handler_init();
  sf_bootloader_hal_init();

  const bootloader_sections_t bootloader_sections = {
		  .app_info = {.address = MEM_APP_INFO_ADDRESS, .size = MEM_APP_INFO_END_ADDRESS - MEM_APP_INFO_ADDRESS },
		  .app = {.address = MEM_APP_START_ADDRESS, .size = MEM_APP_END_ADDRESS - MEM_APP_START_ADDRESS },
		  .upgrade_info = {.address = MEM_UPGRADE_INFO_ADDRESS, .size = MEM_UPGRADE_INFO_END_ADDRESS - MEM_UPGRADE_INFO_ADDRESS },
		  .upgrade = {.address = MEM_UPGRADE_START_ADDRESS, .size = MEM_UPGRADE_END_ADDRESS - MEM_UPGRADE_START_ADDRESS },
  };

  bootloader_init(&bootloader_config, &bootloader_sections);

  if (stay_in_bootloader) {
	  bootloader_stay(true);
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  uint8_t app_status = bootloader_app_status();

	  can_message_handler_task(BOOTLOADER_ECU_CODE_ID, app_status, BOOTLOADER_VERSION);
	  uint32_t time = sf_bootloader_hal_get_1ms_counter();
	  bootloader_tick(time);


	  if( (time - led_timer) >= BOOTLOADER_LED_TIME_TOGGLE)
	  {
		  led_timer = sf_bootloader_hal_get_1ms_counter();

		  if( gpio_get( LED1 ) == 0 ) {
			  gpio_set( LED1 );
			  gpio_clear( LED2 );
		  }
		  else
		  {
			  gpio_clear( LED1 );
			  gpio_set( LED2 );
		  }
	  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_5)
  {
  }

  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
  while (LL_PWR_IsActiveFlag_VOS() == 0)
  {
  }
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  LL_RCC_HSI_SetCalibTrimming(64);
  LL_RCC_HSI_SetDivider(LL_RCC_HSI_DIV_1);
  LL_RCC_PLL1_SetSource(LL_RCC_PLL1SOURCE_HSI);
  LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_8_16);
  LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
  LL_RCC_PLL1_SetM(4);
  LL_RCC_PLL1_SetN(30);
  LL_RCC_PLL1_SetP(2);
  LL_RCC_PLL1_SetQ(2);
  LL_RCC_PLL1_SetR(2);
  LL_RCC_PLL1Q_Enable();
  LL_RCC_PLL1P_Enable();
  LL_RCC_PLL1_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL1_IsReady() != 1)
  {
  }

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1)
  {
  }

  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_1);
  LL_SetSystemCoreClock(240000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{

  /* Disables the MPU */
  LL_MPU_Disable();

  /** Initializes and configures the Attribute 0 and the memory to be protected
  */
  LL_MPU_ConfigAttributes(LL_MPU_ATTRIBUTES_NUMBER0, LL_MPU_NOT_CACHEABLE);
  /* Enables the MPU */
  LL_MPU_Enable(LL_MPU_CTRL_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
