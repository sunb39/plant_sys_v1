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


/* PUMP_DEMO_ENABLE（水泵联调演示开关）
 * 作用：
 * 1. =1 时，采用联调演示模式，不走真实自动浇水逻辑
 * 2. =0 时，后续可以恢复 APP_Control_Run() 自动控制
 */
#define PUMP_DEMO_ENABLE        (1U)

/* 水泵联调演示参数 */
#define PUMP_TEST_ON_MS         (2000U)   /* 水泵运行 2 秒 */
#define PUMP_TEST_OFF_MS        (15000U)  /* 水泵停止 15 秒 */

/* 水泵启动前提示保留时间
 * 作用：让 OLED 有时间显示 “WATERING...” 提示
 */
#define PUMP_PRESTART_MSG_MS    (800U)

/* 水泵停止后恢复等待时间
 * 作用：等待电机干扰衰减后，再恢复 I2C 与 OLED
 */
#define PUMP_RECOVER_DELAY_MS   (500U)


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

SensorData_t  g_sensor;        /* g_sensor（实时传感器数据） */
SystemParam_t g_param;         /* g_param（系统参数） */
SystemState_t g_state;         /* g_state（系统状态） */

uint32_t g_sensor_tick  = 0U;  /* g_sensor_tick（传感器调度时间戳） */
uint32_t g_display_tick = 0U;  /* g_display_tick（显示调度时间戳） */

/* 水泵联调状态变量 */
uint8_t  g_i2c_pause = 0U;         /* g_i2c_pause（I2C暂停标志，1表示暂停 OLED/BH1750） */
uint32_t g_pump_tick = 0U;         /* g_pump_tick（水泵节拍时间戳） */
uint8_t  g_pump_phase_on = 0U;     /* g_pump_phase_on（水泵当前阶段，1开/0关） */
uint8_t  g_pump_recover_req = 0U;  /* g_pump_recover_req（停泵后恢复显示请求） */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

static void APP_DefaultParam_Init(SystemParam_t *param);
static void OLED_ShowReady(void);
static void OLED_ShowWatering(void);
static void OLED_I2C_BusRecover(void);
static void OLED_RecoverAfterPump(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* OLED_ShowReady（显示系统就绪界面）
 * 作用：系统初始化完成后显示 READY
 * 说明：用于联调阶段提示系统已经进入待命状态
 */
static void OLED_ShowReady(void)
{
    BSP_OLED_Clear();
    BSP_OLED_ShowString(0, 0, "PLANT SYS");
    BSP_OLED_ShowString(0, 2, "READY");
    BSP_OLED_Refresh();
}

/* OLED_ShowWatering（显示浇水提示界面）
 * 作用：在水泵启动前显示 WATERING...
 * 说明：用于提醒用户系统即将执行补水动作
 */
static void OLED_ShowWatering(void)
{
    BSP_OLED_Clear();
    BSP_OLED_ShowString(0, 0, "PLANT SYS");
    BSP_OLED_ShowString(0, 2, "WATERING...");
    BSP_OLED_Refresh();
}

/* OLED_I2C_BusRecover（I2C总线恢复）
 * 作用：
 * 1. 当水泵启动后电磁干扰导致 I2C 总线异常时，尝试通过手动时钟脉冲恢复总线
 * 2. 该函数会先释放 I2C 外设，再将 PB6/PB7 临时改成 GPIO 开漏输出
 * 3. 通过给 SCL（PB6）输出 9 个脉冲，尝试释放被挂住的 SDA（PB7）
 * 4. 最后重新初始化 I2C1 外设
 *
 * 说明：
 * 1. PB6 = I2C1_SCL
 * 2. PB7 = I2C1_SDA
 * 3. 这是典型的 I2C 总线恢复方法
 */
static void OLED_I2C_BusRecover(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint8_t i = 0;

    /* 先反初始化 I2C 外设，释放 PB6 / PB7 */
    HAL_I2C_DeInit(&hi2c1);

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 将 PB6 / PB7 临时改成开漏输出 */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* 先释放总线为高电平 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1);

    /* 对 SCL 输出 9 个时钟脉冲，尝试释放挂住的从设备 */
    for (i = 0; i < 9; i++)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(1);
    }

    /* 产生一次 STOP 条件：SCL 高时，SDA 从低释放到高 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1);

    /* 重新初始化 I2C1 */
    MX_I2C1_Init();
}

/* OLED_RecoverAfterPump（停泵后恢复 OLED 显示）
 * 作用：
 * 1. 停泵后先恢复 I2C 总线
 * 2. 再重新初始化 OLED
 * 3. 显示 “PUMP DONE”
 * 4. 最终重新进入 READY 界面
 *
 * 说明：
 * 这是当前联调阶段采用的软件抗干扰恢复策略
 */
static void OLED_RecoverAfterPump(void)
{
    OLED_I2C_BusRecover();

    HAL_Delay(10);

    BSP_OLED_Init();
    BSP_OLED_Clear();
    BSP_OLED_ShowString(0, 0, "PLANT SYS");
    BSP_OLED_ShowString(0, 2, "PUMP DONE");
    BSP_OLED_Refresh();

    HAL_Delay(1000);

    OLED_ShowReady();
}

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




//int main(void)
//{

//  /* USER CODE BEGIN 1 */

//  /* USER CODE END 1 */

//  /* MCU Configuration--------------------------------------------------------*/

//  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
//  HAL_Init();

//  /* USER CODE BEGIN Init */

//  /* USER CODE END Init */

//  /* Configure the system clock */
//  SystemClock_Config();

//  /* USER CODE BEGIN SysInit */

//  /* USER CODE END SysInit */

//  /* Initialize all configured peripherals */
//  MX_GPIO_Init();
//  MX_ADC1_Init();
//  MX_I2C1_Init();
//  MX_TIM2_Init();
//  MX_USART1_UART_Init();
//  /* USER CODE BEGIN 2 */
//  /* 初始化 OLED */
//	BSP_OLED_Init();
//	BSP_BH1750_Init();
//	BSP_DHT11_Init();
//	BSP_Soil_Init();
//	BSP_Debug_Init();
//	/* 初始化系统默认参数 */
//	APP_DefaultParam_Init(&g_param);

//	/* 初始化系统状态 */
//	APP_Control_Init(&g_state);

//	/* 初始化菜单 */
//	APP_Menu_Init();

//	/* 初始化显示层 */
//	APP_Display_Init();

//	/* 初始化传感器数据 */
//	APP_Sensor_Init(&g_sensor);

//	/* 初始化云端通信层 */
//	//APP_Cloud_Init(&g_state);
//	/* 初始化执行器输出 */
//	BSP_Output_Init();

//	
//	/* 初始化 pH 软件换算模块 */
//	APP_PH_Init();
//	/* 初始化软件调度时间戳 */
//	g_sensor_tick  = HAL_GetTick();
//	g_display_tick = HAL_GetTick();
//	g_cloud_tick   = HAL_GetTick();
//	/* 关闭 PC13 板载LED
//	 * 说明：多数 Blue Pill 开发板的 PC13 板载灯为低电平点亮，高电平熄灭
//	 */
//	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
//	/* 系统上电后先等待 5 秒，让 OLED 和系统稳定 */
//	HAL_Delay(5000);
//	/* 初始化水泵联调节拍
// * 说明：系统启动后先进入停泵阶段，等待整机稳定
// */
//g_pump_tick = HAL_GetTick();
//g_pump_phase_on = 0U;
//g_state.pump_on = 0U;
//g_i2c_pause = 0U;
//g_last_pump_on = 0U;
//  /* USER CODE END 2 */

//  /* Infinite loop */
//  /* USER CODE BEGIN WHILE */

//while (1)
//{
//    uint32_t now_tick = HAL_GetTick();

//    /* =========================================
//     * 任务1：水泵联调节拍控制
//     * 说明：
//     * 1. 当前处于整机联调阶段，先不采用真实自动浇水阈值
//     * 2. 采用“开2秒、停15秒”的方式验证整机执行器链路
//     * 3. 水泵运行期间暂停 I2C 相关任务，降低电机干扰影响
//     * =========================================
//     */
//    if (g_pump_phase_on == 0U)
//    {
//        /* 当前处于停泵阶段 */
//        if ((now_tick - g_pump_tick) >= PUMP_TEST_OFF_MS)
//        {
//            g_pump_phase_on = 1U;
//            g_pump_tick = now_tick;
//            g_state.pump_on = 1U;

//            /* 水泵启动时，暂停所有 I2C 相关任务 */
//            g_i2c_pause = 1U;
//        }
//    }
//    else
//    {
//        /* 当前处于开泵阶段 */
//        if ((now_tick - g_pump_tick) >= PUMP_TEST_ON_MS)
//        {
//            g_pump_phase_on = 0U;
//            g_pump_tick = now_tick;
//            g_state.pump_on = 0U;

//            /* 水泵停止后，先等待干扰衰减 */
//            HAL_Delay(PUMP_RECOVER_DELAY_MS);

//            /* 重新初始化 OLED，恢复显示链路 */
//            BSP_OLED_Init();
//            APP_Display_Init();

//            /* 恢复 I2C 任务 */
//            g_i2c_pause = 0U;
//        }
//    }

//    /* 当前阶段先关闭补光和蜂鸣器，减少联调变量 */
//    g_state.light_on = 0U;
//    g_state.beep_on  = 0U;

//    /* =========================================
//     * 任务2：执行器输出更新
//     * 说明：
//     * 根据 g_state 状态位控制水泵、补光和蜂鸣器
//     * =========================================
//     */
//    BSP_Output_Update(&g_state);

//    /* =========================================
//     * 任务3：传感器数据更新
//     * 说明：
//     * 1. 水泵运行期间不访问 I2C
//     * 2. 因为 APP_Sensor_Update() 内部会访问 BH1750（I2C）
//     * 3. 因此开泵时直接跳过本次传感器更新
//     * =========================================
//     */
//    if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
//    {
//        g_sensor_tick = now_tick;

//        if (g_i2c_pause == 0U)
//        {
//            APP_Sensor_Update(&g_sensor);
//        }
//    }

//    /* =========================================
//     * 任务4：控制逻辑计算
//     * 说明：
//     * 当前联调阶段仍保留控制逻辑运行，
//     * 但 pump_on 由上面的测试节拍强制控制
//     * =========================================
//     */
//    //APP_Control_Run(&g_sensor, &g_param, &g_state);

//    /* =========================================
//     * 任务5：OLED 显示刷新
//     * 说明：
//     * 仅在 I2C 未暂停时刷新 OLED
//     * =========================================
//     */
//    if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
//    {
//        g_display_tick = now_tick;

//        if (g_i2c_pause == 0U)
//        {
//            APP_Display_Show(APP_Menu_GetPage(), &g_sensor, &g_param, &g_state);
//        }
//    }

//    HAL_Delay(10);
//}
//  /* USER CODE END 3 */
//}
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

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
  BSP_OLED_Init();
  BSP_BH1750_Init();
  BSP_DHT11_Init();
  BSP_Soil_Init();
  BSP_Debug_Init();

  /* =========================
   * 上层参数与状态初始化
   * =========================
   */
  APP_DefaultParam_Init(&g_param);
  APP_Control_Init(&g_state);
  APP_Menu_Init();
  APP_Display_Init();
  APP_Sensor_Init(&g_sensor);

  /* 当前联调阶段，云端暂不接入，避免增加变量 */
  /* APP_Cloud_Init(&g_state); */

  /* 执行器初始化 */
  BSP_Output_Init();

  /* pH 软件模块初始化（当前可先保留接口） */
  APP_PH_Init();

  /* 调度时间戳初始化 */
  g_sensor_tick  = HAL_GetTick();
  g_display_tick = HAL_GetTick();

  /* 关闭 PC13 板载 LED
   * 说明：多数 Blue Pill 开发板为低电平点亮、高电平熄灭
   */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /* 初始状态：所有执行器关闭 */
  g_state.pump_on  = 0U;
  g_state.light_on = 0U;
  g_state.beep_on  = 0U;

  /* 初始化联调状态变量 */
  g_i2c_pause = 0U;
  g_pump_phase_on = 0U;
  g_pump_recover_req = 0U;
  g_pump_tick = HAL_GetTick();

  /* 系统上电后先显示 READY，确认 OLED 正常 */
  OLED_ShowReady();

  /* 给系统一个稳定时间 */
  HAL_Delay(3000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      uint32_t now_tick = HAL_GetTick();

#if PUMP_DEMO_ENABLE
      /* =========================================
       * 任务1：水泵联调演示节拍控制
       * 说明：
       * 1. 当前属于联调版，不直接走自动浇水逻辑
       * 2. 采用“停15秒 -> 提示 -> 开2秒 -> 恢复显示”的节拍
       * 3. 水泵运行期间暂停所有 I2C 相关任务
       * =========================================
       */
      if (g_pump_phase_on == 0U)
      {
          /* 当前处于停泵阶段 */
          if ((now_tick - g_pump_tick) >= PUMP_TEST_OFF_MS)
          {
              /* 先显示即将浇水提示 */
              OLED_ShowWatering();
              HAL_Delay(PUMP_PRESTART_MSG_MS);

              /* 进入开泵阶段 */
              g_pump_phase_on = 1U;
              g_pump_tick = HAL_GetTick();
              g_state.pump_on = 1U;

              /* 开泵后暂停所有 I2C 任务 */
              g_i2c_pause = 1U;
          }
      }
      else
      {
          /* 当前处于开泵阶段 */
          if ((now_tick - g_pump_tick) >= PUMP_TEST_ON_MS)
          {
              /* 关泵 */
              g_state.pump_on = 0U;
              g_pump_phase_on = 0U;
              g_pump_tick = HAL_GetTick();

              /* 标记需要在停泵后恢复 OLED */
              g_pump_recover_req = 1U;
          }
      }
#else
      /* =========================================
       * 正式自动控制逻辑（后续启用）
       * 说明：
       * 当联调结束后，可将 PUMP_DEMO_ENABLE 改为 0，
       * 恢复 APP_Control_Run() 自动控制执行器。
       * =========================================
       */
      APP_Control_Run(&g_sensor, &g_param, &g_state);
#endif

      /* 当前联调阶段先关闭补光和蜂鸣器，减少变量 */
      g_state.light_on = 0U;
      g_state.beep_on  = 0U;

      /* =========================================
       * 任务2：执行器输出更新
       * 说明：
       * 根据 g_state 状态位控制水泵、补光和蜂鸣器
       * =========================================
       */
      BSP_Output_Update(&g_state);

      /* =========================================
       * 任务3：停泵后恢复 OLED 显示
       * 说明：
       * 1. 等待水泵停止后干扰衰减
       * 2. 先恢复 I2C 总线，再恢复 OLED
       * =========================================
       */
      if (g_pump_recover_req == 1U)
      {
          HAL_Delay(PUMP_RECOVER_DELAY_MS);

          OLED_RecoverAfterPump();

          /* 恢复 I2C 任务 */
          g_i2c_pause = 0U;
          g_pump_recover_req = 0U;

          /* 防止恢复后立刻被下一次刷新覆盖 */
          g_display_tick = HAL_GetTick();
      }

      /* =========================================
       * 任务4：传感器数据更新
       * 说明：
       * 1. 水泵运行期间不访问 I2C
       * 2. APP_Sensor_Update() 内部会读取 BH1750（I2C）
       * 3. 因此开泵时直接跳过本次采样
       * =========================================
       */
      if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
      {
          g_sensor_tick = now_tick;

          if (g_i2c_pause == 0U)
          {
              APP_Sensor_Update(&g_sensor);
          }
      }

      /* =========================================
       * 任务5：OLED 显示刷新
       * 说明：
       * 仅在 I2C 未暂停时刷新 OLED
       * =========================================
       */
      if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
      {
          g_display_tick = now_tick;

          if (g_i2c_pause == 0U)
          {
              APP_Display_Show(APP_Menu_GetPage(), &g_sensor, &g_param, &g_state);
          }
      }

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

