


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
#include "bsp_oled.h"
#include "bsp_output.h"
#include "app_ph.h"
#include "bsp_bh1750.h"
#include "bsp_dht11.h"
#include "bsp_soil.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* LIGHT_DEBUG_ENABLE（自动补光联调开关）
 * 作用：
 * 1. =1 时，进入“自动补光联调版”
 * 2. =0 时，可按以后需求再恢复为其它联调逻辑
 */
#define LIGHT_DEBUG_ENABLE      (1U)

/* LIGHT_TEST_TH（自动补光测试阈值，单位 lux）
 * 作用：
 * 1. 当光照值低于该阈值时，系统判定环境偏暗
 * 2. 当前阶段先设为 500 lux，方便手遮挡和手机灯照射测试
 */
#define LIGHT_TEST_TH           (100.0f)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* 全局传感器数据结构体
 * g_sensor：保存当前温度、湿度、光照、土壤湿度、pH 等实时数据
 */
SensorData_t  g_sensor;

/* 全局系统参数结构体
 * g_param：保存阈值、自动功能使能等控制参数
 */
SystemParam_t g_param;

/* 全局系统状态结构体
 * g_state：保存水泵、补光、蜂鸣器、通信状态等运行状态
 */
SystemState_t g_state;

/* 周期任务时间戳 */
uint32_t g_sensor_tick  = 0U;   /* 传感器采样时间戳 */
uint32_t g_display_tick = 0U;   /* OLED 刷新时间戳 */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
static void APP_DefaultParam_Init(SystemParam_t *param);
static void OLED_ShowReady(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* OLED_ShowReady（显示系统就绪页面）
 * 作用：
 * 1. 系统初始化完成后，在 OLED 上显示 READY
 * 2. 用于提示当前程序已正常启动
 */
static void OLED_ShowReady(void)
{
    BSP_OLED_Clear();
    BSP_OLED_ShowString(0, 0, "PLANT SYS");
    BSP_OLED_ShowString(0, 2, "READY");
    BSP_OLED_Refresh();
}

/* APP_DefaultParam_Init（默认参数初始化）
 * 作用：
 * 1. 给系统参数结构体加载默认阈值
 * 2. 打开自动浇水、自动补光、报警功能的总使能
 * 说明：
 * 这些默认值主要来自 plant_config.h 中的宏定义
 */
static void APP_DefaultParam_Init(SystemParam_t *param)
{
    if (param == 0)
    {
        return;
    }

    /* 载入默认阈值 */
    param->soil_low_th   = DEF_SOIL_LOW_TH;
    param->light_low_th  = DEF_LIGHT_LOW_TH;
    param->ph_low_th     = DEF_PH_LOW_TH;
    param->ph_high_th    = DEF_PH_HIGH_TH;

    /* 打开默认功能使能 */
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

  /* 复位所有外设，初始化 Flash 接口和 SysTick */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* 配置系统时钟 */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* 初始化所有已配置外设 */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */

  /* =========================
   * 底层模块初始化
   * 说明：
   * 先初始化底层 BSP，再初始化上层 APP
   * =========================
   */
  BSP_OLED_Init();      /* OLED 初始化 */
  BSP_BH1750_Init();    /* 光照传感器初始化 */
  BSP_DHT11_Init();     /* 温湿度传感器初始化 */
  BSP_Soil_Init();      /* 土壤湿度 ADC 初始化 */
  BSP_Debug_Init();     /* 串口调试输出初始化 */

  /* =========================
   * 上层参数与状态初始化
   * =========================
   */
  APP_DefaultParam_Init(&g_param);   /* 加载系统默认参数 */
  APP_Control_Init(&g_state);        /* 初始化系统状态 */
  APP_Menu_Init();                   /* 初始化菜单页面 */
  APP_Display_Init();                /* 初始化显示缓存 */
  APP_Sensor_Init(&g_sensor);        /* 初始化传感器数据默认值 */

  /* 当前阶段云端不上线，先保留接口 */
  /* APP_Cloud_Init(&g_state); */

  /* 执行器初始化
   * 作用：上电后先关闭水泵、补光、蜂鸣器，避免误动作
   */
  BSP_Output_Init();

  /* pH 软件模块初始化
   * 说明：当前 pH 仍可先保留软件接口，后续再接真实硬件
   */
  APP_PH_Init();

  /* 初始化周期任务时间戳 */
  g_sensor_tick  = HAL_GetTick();
  g_display_tick = HAL_GetTick();

  /* 关闭 PC13 板载 LED
   * 说明：
   * 大多数 Blue Pill 板载 LED 为低电平点亮、高电平熄灭
   */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /* 设置当前阶段的自动补光测试阈值
   * 说明：
   * 1. 当前先设为 100 lux，方便联调
   * 2. 你后面可根据现场环境改成更合适的值
   */

  g_param.light_low_th = LIGHT_TEST_TH;   /* light_low_th（补光下阈值）当前按室内实测设为 100 lux */
  /* 为了当前联调更稳定，先明确状态默认值 */
  g_state.pump_on  = 0U;
  g_state.light_on = 0U;
  g_state.beep_on  = 0U;

  /* 显示 READY 页面，提示系统启动完成 */
  OLED_ShowReady();

  /* 稳定等待一小段时间 */
  HAL_Delay(1000);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      uint32_t now_tick = HAL_GetTick();   /* now_tick（当前系统时间戳） */

#if LIGHT_DEBUG_ENABLE
      /* =========================================
       * 任务1：更新传感器数据
       * 说明：
       * 1. 定时读取 DHT11、BH1750、土壤湿度等数据
       * 2. 当前 pH 可先继续走 mock 或后续再接真实采样
       * =========================================
       */
      if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
      {
          g_sensor_tick = now_tick;
          APP_Sensor_Update(&g_sensor);
      }

      /* =========================================
       * 任务2：执行自动控制逻辑
       * 说明：
       * 1. 让系统根据实时数据和阈值计算输出状态
       * 2. 例如：光照低于阈值时，light_on 会被置 1
       * =========================================
       */
      APP_Control_Run(&g_sensor, &g_param, &g_state);

      /* =========================================
       * 任务3：当前阶段只验证自动补光
       * 说明：
       * 1. 先强制关闭水泵，避免与当前联调目标无关的动作
       * 2. 先强制关闭蜂鸣器，减少干扰
       * 3. 保留 light_on，让 PB1 根据光照自动亮灭
       * =========================================
       */
      g_state.pump_on = 0U;
      g_state.beep_on = 0U;

      /* =========================================
       * 任务4：更新执行器输出
       * 说明：
       * 1. 根据 g_state 中的状态位，控制实际 GPIO 输出
       * 2. 当前阶段主要观察 PB1 补光输出是否正确
       * =========================================
       */
      BSP_Output_Update(&g_state);

      /* =========================================
       * 任务5：刷新 OLED 显示
       * 说明：
       * 1. 显示当前温湿度、光照、土壤湿度以及输出状态
       * 2. 便于观察 light_on 是否随 light_lux 变化
       * =========================================
       */
      if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
      {
          g_display_tick = now_tick;
          APP_Display_Show(APP_Menu_GetPage(), &g_sensor, &g_param, &g_state);
      }

#else
      /* 后续如果不走自动补光联调，可在这里扩展其它逻辑 */
#endif

      HAL_Delay(10);
  }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection    = RCC_ADCPCLK2_DIV6;
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
  /* 这里可根据需要加入调试打印 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

