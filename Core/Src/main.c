/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "plant_types.h"
#include "plant_config.h"
#include "app_sensor.h"
#include "app_control.h"
#include "app_display.h"
#include "app_menu.h"
#include "app_cloud.h"
#include "bsp_debug.h"
#include "app_display.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* USER CODE BEGIN PV */
SensorData_t  g_sensor;        /* g_sensor（实时传感器数据） */
SystemParam_t g_param;         /* g_param（系统参数） */
SystemState_t g_state;         /* g_state（系统状态） */

uint32_t g_sensor_tick  = 0U;  /* g_sensor_tick（传感器调度时间戳） */
uint32_t g_display_tick = 0U;  /* g_display_tick（显示调度时间戳） */
uint32_t g_cloud_tick   = 0U;  /* g_cloud_tick（云端调度时间戳） */
/* USER CODE END PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void APP_DefaultParam_Init(SystemParam_t *param);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* APP_DefaultParam_Init（默认参数初始化）
 * 作用：系统上电时给所有阈值和使能参数赋初值
 * 答辩可说：该函数用于集中管理默认控制参数，便于系统初始化与后续参数恢复。
 */
static void APP_DefaultParam_Init(SystemParam_t *param)
{
    if (param == 0)
    {
        return;
    }

    param->soil_low_th   = DEF_SOIL_LOW_TH;
    param->light_low_th  = DEF_LIGHT_LOW_TH;
    param->ph_low_th     = DEF_PH_LOW_TH;
    param->ph_high_th    = DEF_PH_HIGH_TH;
    param->auto_water_en = 1U;
    param->auto_light_en = 1U;
    param->alarm_en      = 1U;
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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	BSP_Debug_Init();
	/* 初始化系统默认参数 */
	APP_DefaultParam_Init(&g_param);

	/* 初始化系统状态 */
	APP_Control_Init(&g_state);

	/* 初始化菜单 */
	APP_Menu_Init();

	/* 初始化显示层 */
	APP_Display_Init();

	/* 初始化传感器数据 */
	APP_Sensor_Init(&g_sensor);

	/* 初始化云端通信层 */
	APP_Cloud_Init(&g_state);

	/* 初始化软件调度时间戳 */
	g_sensor_tick  = HAL_GetTick();
	g_display_tick = HAL_GetTick();
	g_cloud_tick   = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
{
    uint32_t now_tick = HAL_GetTick();

    /* 任务1：传感器数据更新 + 控制逻辑计算 */
    if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
    {
        g_sensor_tick = now_tick;

        APP_Sensor_Update(&g_sensor);
        APP_Control_Run(&g_sensor, &g_param, &g_state);
    }

    /* 任务2：界面显示刷新 */
    if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
    {
        g_display_tick = now_tick;

        APP_Display_Show(APP_Menu_GetPage(), &g_sensor, &g_param, &g_state);
    }

    /* 任务3：云端数据上传 */
    if ((now_tick - g_cloud_tick) >= CLOUD_PERIOD_MS)
    {
        g_cloud_tick = now_tick;

        APP_Cloud_Upload(&g_sensor, &g_state);
				BSP_Debug_Printf("T_x10=%ld,H_x10=%ld,L_x10=%ld,S_x10=%ld,PH_x100=%ld,P=%d,LED=%d,BEEP=%d\r\n",
                 (long)(g_sensor.air_temp * 10.0f),
                 (long)(g_sensor.air_humi * 10.0f),
                 (long)(g_sensor.light_lux * 10.0f),
                 (long)(g_sensor.soil_moisture * 10.0f),
                 (long)(g_sensor.ph_value * 100.0f),
                 g_state.pump_on,
                 g_state.light_on,
                 g_state.beep_on);

				BSP_Debug_Printf("CloudPacket: %s\r\n", APP_Cloud_GetLastPacket());
    }

    /* 任务4：云端报文解析
     * 这里放在主循环中持续执行，后续适合处理串口接收缓冲区。
     */
    APP_Cloud_Parse();

    /* 适当延时，降低CPU空转占用 */
    HAL_Delay(10);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
