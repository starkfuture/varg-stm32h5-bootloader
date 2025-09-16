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
#include "adc.h"
#include "crc.h"
#include "dac.h"
#include "fdcan.h"
#include "gpdma.h"
#include "icache.h"
#include "spi.h"
#include "tim.h"
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

#define BOOTLOADER_VERSION                  0x03

#if BOARD == STARK_CHARGER

#define BOOTLOADER_ECU_CODE_ID              4U
const public_key_t public_key = {
    .key = {
        0xc8, 0x27, 0xf1, 0xd6, 0xe1, 0x63, 0x76, 0xd4, 0x57, 0xb3, 0x60, 0xef, 0xa6, 0xb4, 0xd2, 0x31,
        0xaf, 0x3d, 0x8b, 0x78, 0x5c, 0xea, 0x16, 0x64, 0x5b, 0x99, 0xb9, 0x8f, 0xfd, 0x9a, 0x74, 0x04,
        0x46, 0x64, 0x2a, 0x26, 0x63, 0x58, 0xbd, 0x42, 0x8b, 0xea, 0xa3, 0x8c, 0x3c, 0x7b, 0x75, 0xa0,
        0xb6, 0x17, 0xe1, 0xea, 0xf6, 0xf0, 0xe3, 0xa1, 0xfc, 0xae, 0x07, 0xf8, 0x7d, 0xec, 0xa8, 0xf4
    }
};
const btea_key_t btea_key = {
    .key = {
        0x777F23C6, 0xFE7B4873, 0xDD595CFF, 0xF65F58EC
    }
};

#elif BOARD == STARK_INVERTER
#define BOOTLOADER_ECU_CODE_ID              5U

const public_key_t public_key = {
    .key = {
    0x98, 0xa8, 0x42, 0xf3, 0xde, 0x12, 0x16, 0x35, 0x5b, 0xd1, 0x31, 0x84, 0x68, 0x77, 0x1a, 0x1a,
    0xf5, 0x15, 0xac, 0x5a, 0x00, 0x44, 0xa0, 0x87, 0xcd, 0x0e, 0x9d, 0x2d, 0xa8, 0x2a, 0x86, 0x73,
    0xce, 0x36, 0xe7, 0xec, 0xfb, 0xe1, 0xfa, 0xc1, 0x59, 0xbd, 0xeb, 0x14, 0xe8, 0x97, 0xc8, 0x49,
    0x80, 0xcf, 0x52, 0x8d, 0x4a, 0x1d, 0xc3, 0xec, 0x29, 0x95, 0x4e, 0x2c, 0xce, 0xea, 0x15, 0x9c
        }
  };
const btea_key_t btea_key = {
    .key = {
        0x5FD7BFC7, 0xF6FF3687, 0xEFF57056, 0x7BD51AEE
    }
}

#else
#error "BOARD needs to be defined"
#endif

#define BOOTLOADER_LED_TIME_TOGGLE		1000

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

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_DAC1_Init();
  MX_FDCAN1_Init();
  MX_ICACHE_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM8_Init();
  MX_TIM13_Init();
  MX_TIM16_Init();
  MX_SPI2_Init();
  MX_SPI4_Init();
  MX_TIM15_Init();
  MX_TIM4_Init();
  MX_TIM14_Init();
  MX_TIM17_Init();
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
	  uint32_t time = get_1ms_counter();
	  bootloader_tick(time);


	  if( (time - led_timer) >= BOOTLOADER_LED_TIME_TOGGLE)
	  {
		  led_timer = get_1ms_counter();

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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_CSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.CSIState = RCC_CSI_ON;
  RCC_OscInitStruct.CSICalibrationValue = RCC_CSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 30;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enables PLL2P clock output
  */
  __HAL_RCC_TIMIC_ENABLE();

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};
  MPU_Attributes_InitTypeDef MPU_AttributesInit = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region 0 and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x08FFF800;
  MPU_InitStruct.LimitAddress = 0x08FFF8FF;
  MPU_InitStruct.AttributesIndex = MPU_ATTRIBUTES_NUMBER0;
  MPU_InitStruct.AccessPermission = MPU_REGION_ALL_RO;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Attribute 0 and the memory to be protected
  */
  MPU_AttributesInit.Number = MPU_ATTRIBUTES_NUMBER0;
  MPU_AttributesInit.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);

  HAL_MPU_ConfigMemoryAttributes(&MPU_AttributesInit);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

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
