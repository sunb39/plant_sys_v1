/////* USER CODE BEGIN Header */
///////**
//////  ******************************************************************************
//////  * @file           : main.c
//////  * @brief          : Main program body
//////  ******************************************************************************
//////  * @attention
//////  *
//////  * Copyright (c) 2026 STMicroelectronics.
//////  * All rights reserved.
//////  *
//////  * This software is licensed under terms that can be found in the LICENSE file
//////  * in the root directory of this software component.
//////  * If no LICENSE file comes with this software, it is provided AS-IS.
//////  *
//////  ******************************************************************************
//////  */
/////* USER CODE END Header */
/////* Includes ------------------------------------------------------------------*/
////#include "main.h"
////#include "adc.h"
////#include "i2c.h"
////#include "tim.h"
////#include "usart.h"
////#include "gpio.h"

/////* Private includes ----------------------------------------------------------*/
/////* USER CODE BEGIN Includes */
////#include "plant_types.h"
////#include "plant_config.h"
////#include "app_sensor.h"
////#include "app_control.h"
////#include "app_display.h"
////#include "app_menu.h"
////#include "app_cloud.h"
////#include "bsp_oled.h"
////#include "bsp_output.h"
////#include "app_ph.h"
////#include "bsp_bh1750.h"
////#include "bsp_dht11.h"
////#include "bsp_soil.h"
////#include "bsp_esp8266.h"
////#include <string.h>

/////* USER CODE END Includes */

/////* Private typedef -----------------------------------------------------------*/
/////* USER CODE BEGIN PTD */
////typedef struct
////{
////    uint8_t active;     /* 是否处于远程强制控制期 */
////    uint8_t value;      /* 强制值 0/1 */
////    uint32_t until;     /* 到期时间戳 */
////} RemoteOverride_t;
/////* USER CODE END PTD */

/////* Private define ------------------------------------------------------------*/
/////* USER CODE BEGIN PD */

/////* 水泵单次运行时间
//// * 说明：检测到土壤过干后，先运行固定时间，再停泵观察
//// */
////#define PUMP_RUN_MS             (2000U)

/////* 水泵启动前提示保留时间 */
////#define PUMP_PRESTART_MSG_MS    (800U)

/////* 水泵停止后恢复等待时间 */
////#define PUMP_RECOVER_DELAY_MS   (500U)

/////* 两次浇水动作之间的最短等待时间
//// * 说明：避免水泵频繁启停
//// */
////#define PUMP_COOLDOWN_MS        (15000U)

/////* 当前整机参数
//// * 说明：
//// * 1. 土壤湿度当前仍用 ADC 原始值，数值越大越干
//// * 2. 光照阈值按你室内实测先设为 100 lux
//// * 3. pH 报警范围当前先设得宽一点，避免误报
//// */
////#define SOIL_DRY_TH_RAW         (4200.0f)
////#define LIGHT_ON_TH_LUX         (100.0f)
////#define PH_ALARM_LOW_TH         (5.0f)
////#define PH_ALARM_HIGH_TH        (7.5f)

////#define ONENET_RETRY_PERIOD_MS   (10000U)   /* OneNET 重连周期 10 秒 */

////#define CLOUD_UPLOAD_PERIOD_MS    (15000U)   /* CLOUD_UPLOAD_PERIOD_MS（上云周期），当前设为5秒 */
////#define REMOTE_OVERRIDE_MS      (10000U)   /* 云端服务控制保持 10 秒 */
/////* USER CODE END PD */

/////* Private macro -------------------------------------------------------------*/
/////* USER CODE BEGIN PM */

/////* USER CODE END PM */

/////* Private variables ---------------------------------------------------------*/

/////* USER CODE BEGIN PV */

////SensorData_t  g_sensor;        /* g_sensor（实时传感器数据） */
////SystemParam_t g_param;         /* g_param（系统参数） */
////SystemState_t g_state;         /* g_state（系统状态） */

////uint32_t g_sensor_tick  = 0U;  /* g_sensor_tick（传感器调度时间戳） */
////uint32_t g_display_tick = 0U;  /* g_display_tick（显示调度时间戳） */

////uint8_t  g_i2c_pause = 0U;          /* g_i2c_pause（I2C暂停标志） */
////uint8_t  g_pump_running = 0U;       /* g_pump_running（水泵实际运行标志） */
////uint32_t g_pump_tick = 0U;          /* g_pump_tick（水泵启动时间戳） */
////uint32_t g_pump_lock_until = 0U;    /* g_pump_lock_until（下次允许开泵的最早时刻） */
////uint32_t g_cloud_tick = 0U;   /* g_cloud_tick（云端上传调度时间戳） */
////RemoteOverride_t g_remote_pump;
////RemoteOverride_t g_remote_light;
////RemoteOverride_t g_remote_beep;

////uint32_t g_onenet_retry_tick = 0U;   /* OneNET 重连调度时间戳 */
/////* USER CODE END PV */

/////* Private function prototypes -----------------------------------------------*/
////void SystemClock_Config(void);
/////* USER CODE BEGIN PFP */

////static void APP_DefaultParam_Init(SystemParam_t *param);
////static void OLED_ShowReady(void);
////static void OLED_ShowWatering(void);
////static void OLED_I2C_BusRecover(void);
////static void OLED_RecoverAfterPump(void);

////static void APP_ClearRemoteOverrides(void);
////static void APP_ApplyRemoteOverrides(uint32_t now_tick, SystemState_t *state);
////static void APP_HandleServiceCommand(const ESP_ServiceCmd_t *cmd, uint32_t now_tick);

/////* USER CODE END PFP */

/////* Private user code ---------------------------------------------------------*/
/////* USER CODE BEGIN 0 */

/////* OLED_ShowReady（显示系统就绪界面） */
////static void OLED_ShowReady(void)
////{
////    BSP_OLED_Clear();
////    BSP_OLED_ShowString(0, 0, "PLANT SYS");
////    BSP_OLED_ShowString(0, 2, "READY");
////    BSP_OLED_Refresh();
////}

/////* OLED_ShowWatering（显示浇水提示界面） */
////static void OLED_ShowWatering(void)
////{
////    BSP_OLED_Clear();
////    BSP_OLED_ShowString(0, 0, "PLANT SYS");
////    BSP_OLED_ShowString(0, 2, "WATERING...");
////    BSP_OLED_Refresh();
////}

/////* OLED_I2C_BusRecover（I2C总线恢复） */
////static void OLED_I2C_BusRecover(void)
////{
////    GPIO_InitTypeDef GPIO_InitStruct = {0};
////    uint8_t i = 0U;

////    HAL_I2C_DeInit(&hi2c1);

////    __HAL_RCC_GPIOB_CLK_ENABLE();

////    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
////    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
////    GPIO_InitStruct.Pull = GPIO_PULLUP;
////    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
////    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

////    /* 释放总线 */
////    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
////    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
////    HAL_Delay(1);

////    /* SCL 输出 9 个时钟脉冲 */
////    for (i = 0U; i < 9U; i++)
////    {
////        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
////        HAL_Delay(1);
////        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
////        HAL_Delay(1);
////    }

////    /* 产生一次 STOP 条件 */
////    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
////    HAL_Delay(1);
////    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
////    HAL_Delay(1);
////    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
////    HAL_Delay(1);

////    MX_I2C1_Init();
////}

/////* OLED_RecoverAfterPump（停泵后恢复 OLED 显示） */
////static void OLED_RecoverAfterPump(void)
////{
////    OLED_I2C_BusRecover();

////    HAL_Delay(10);

////    BSP_OLED_Init();
////    BSP_OLED_Clear();
////    BSP_OLED_ShowString(0, 0, "PLANT SYS");
////    BSP_OLED_ShowString(0, 2, "PUMP DONE");
////    BSP_OLED_Refresh();

////    HAL_Delay(1000);

////    OLED_ShowReady();
////}

/////* APP_DefaultParam_Init（默认参数初始化） */
////static void APP_DefaultParam_Init(SystemParam_t *param)
////{
////    if (param == 0)
////    {
////        return;
////    }

////    param->soil_low_th   = DEF_SOIL_LOW_TH;
////    param->light_low_th  = DEF_LIGHT_LOW_TH;
////    param->ph_low_th     = DEF_PH_LOW_TH;
////    param->ph_high_th    = DEF_PH_HIGH_TH;
////    param->auto_water_en = 1U;
////    param->auto_light_en = 1U;
////    param->alarm_en      = 1U;
////}

/////* APP_ClearRemoteOverrides（清空远程强制控制） */
////static void APP_ClearRemoteOverrides(void)
////{
////    memset(&g_remote_pump,  0, sizeof(g_remote_pump));
////    memset(&g_remote_light, 0, sizeof(g_remote_light));
////    memset(&g_remote_beep,  0, sizeof(g_remote_beep));
////}

/////* APP_HandleServiceCommand（处理 OneNET 服务调用）
//// * 说明：
//// * 1. 收到服务调用后，不直接永久改自动控制逻辑
//// * 2. 只做一个 10 秒的远程强制控制窗口
//// * 3. 到期后系统重新回到自动控制
//// */
////static void APP_HandleServiceCommand(const ESP_ServiceCmd_t *cmd, uint32_t now_tick)
////{
////    if (cmd == 0)
////    {
////        return;
////    }

////    switch (cmd->type)
////    {
//////        case ESP_SERVICE_SET_PUMP:
//////            g_remote_pump.active = 1U;
//////            g_remote_pump.value  = cmd->value;
//////            g_remote_pump.until  = now_tick + REMOTE_OVERRIDE_MS;
//////            break;

//////        case ESP_SERVICE_SET_LIGHT:
//////            g_remote_light.active = 1U;
//////            g_remote_light.value  = cmd->value;
//////            g_remote_light.until  = now_tick + REMOTE_OVERRIDE_MS;
//////            break;
//////			/* ==========================================================
//////             * 【补光灯专属测试代码】
//////             * 作用：把解析到的值直接打印到 OLED 第五行！
//////             * ========================================================== */
//////            if (cmd->value == 1) {
//////                BSP_OLED_ShowString(0, 4, "LIGHT CMD: ON "); 
//////            } else {
//////                BSP_OLED_ShowString(0, 4, "LIGHT CMD: OFF"); 
//////            }
//////            BSP_OLED_Refresh();
//////            /* ========================================================== */
//////            break;
////case ESP_SERVICE_SET_LIGHT:
////            g_remote_light.active = 1U;
////            g_remote_light.value  = cmd->value; 
////            g_remote_light.until  = now_tick + REMOTE_OVERRIDE_MS;
////            
////            /* ==========================================================
////             * 【防覆盖终极测试代码】
////             * ========================================================== */
////            if (cmd->value == 1) {
////                BSP_OLED_ShowString(0, 4, "LIGHT CMD: ON "); 
////            } else {
////                BSP_OLED_ShowString(0, 4, "LIGHT CMD: OFF"); 
////            }
////            BSP_OLED_Refresh();
////            
////            // 1. 点亮主板上的 PC13 测试灯（双保险，如果屏幕没反应，看主板灯！）
////            HAL_GPIO_WritePin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin, GPIO_PIN_SET);
////            
////            // 2. 强行死等 2 秒！不让主循环去覆盖屏幕！
////            HAL_Delay(2000); 
////            
////            // 3. 2秒后关掉主板测试灯，恢复正常运行
////            HAL_GPIO_WritePin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin, GPIO_PIN_RESET);
////            /* ========================================================== */
////            break;
//////        case ESP_SERVICE_SET_BEEP:
//////            g_remote_beep.active = 1U;
//////            g_remote_beep.value  = cmd->value;
//////            g_remote_beep.until  = now_tick + REMOTE_OVERRIDE_MS;
//////            break;

////        default:
////            break;
////    }
////}



/////* APP_ApplyRemoteOverrides（应用远程强制控制） */
////static void APP_ApplyRemoteOverrides(uint32_t now_tick, SystemState_t *state)
////{
////    if (state == 0)
////    {
////        return;
////    }

////    if ((g_remote_pump.active != 0U) && (now_tick < g_remote_pump.until))
////    {
////        state->pump_on = g_remote_pump.value;
////    }
////    else
////    {
////        g_remote_pump.active = 0U;
////    }

////    if ((g_remote_light.active != 0U) && (now_tick < g_remote_light.until))
////    {
////        state->light_on = g_remote_light.value;
////    }
////    else
////    {
////        g_remote_light.active = 0U;
////    }

////    if ((g_remote_beep.active != 0U) && (now_tick < g_remote_beep.until))
////    {
////        state->beep_on = g_remote_beep.value;
////    }
////    else
////    {
////        g_remote_beep.active = 0U;
////    }
////}

/////* USER CODE END 0 */

/////**
////  * @brief  The application entry point.
////  * @retval int
////  */
////int main(void)
////{

////  /* USER CODE BEGIN 1 */

////  /* USER CODE END 1 */

////  /* MCU Configuration--------------------------------------------------------*/

////  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
////  HAL_Init();

////  /* USER CODE BEGIN Init */

////  /* USER CODE END Init */

////  /* Configure the system clock */
////  SystemClock_Config();

////  /* USER CODE BEGIN SysInit */

////  /* USER CODE END SysInit */

////  /* Initialize all configured peripherals */
////  MX_GPIO_Init();
////  MX_ADC1_Init();
////  MX_I2C1_Init();
////  MX_TIM2_Init();
////  MX_USART1_UART_Init();
////  /* USER CODE BEGIN 2 */

//// /* =========================
////   * 底层模块初始化
////   * =========================
////   */

////  BSP_OLED_Init();
////  BSP_BH1750_Init();
////  BSP_DHT11_Init();
////  BSP_Soil_Init();
////  /* 注意：USART1 已经给 ESP8266 用，不能再初始化调试串口输出 */
////  /* =========================
////   * 上层参数与状态初始化
////   * =========================
////   */
////  APP_DefaultParam_Init(&g_param);
////  APP_Control_Init(&g_state);
////  APP_Menu_Init();
////  APP_Display_Init();
////  APP_Sensor_Init(&g_sensor);
////  BSP_Output_Init();
////  APP_PH_Init();
////  
////	  /* 云端与 ESP8266 初始化 */
////	APP_Cloud_Init(&g_state);
////	BSP_ESP8266_Init();
////	APP_ClearRemoteOverrides();

////	g_cloud_tick = HAL_GetTick();
////	g_onenet_retry_tick = HAL_GetTick();

////	g_state.wifi_ok  = 0U;
////	g_state.cloud_ok = 0U;
////  
////  g_sensor_tick  = HAL_GetTick();
////  g_display_tick = HAL_GetTick();

////	OLED_ShowReady();
////	HAL_Delay(300);
////  /* 关闭 PC13 板载 LED */
////  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

////  /* =========================
////   * 当前整机工作参数
////   * =========================
////   */
////  g_param.soil_low_th   = SOIL_DRY_TH_RAW;     /* 土壤干燥阈值（ADC原始值） */
////  g_param.light_low_th  = LIGHT_ON_TH_LUX;     /* 自动补光开启阈值 */
////  g_param.ph_low_th     = PH_ALARM_LOW_TH;     /* pH 报警下限 */
////  g_param.ph_high_th    = PH_ALARM_HIGH_TH;    /* pH 报警上限 */
////  g_param.auto_water_en = 1U;
////  g_param.auto_light_en = 1U;
////  g_param.alarm_en      = 1U;

////  /* 初始状态 */
////  g_state.pump_on  = 0U;
////  g_state.light_on = 0U;
////  g_state.beep_on  = 0U;

////  g_i2c_pause      = 0U;
////  g_pump_running   = 0U;
////  g_pump_tick      = 0U;
////  g_pump_lock_until = HAL_GetTick();

//////  OLED_ShowReady();
//////  HAL_Delay(1000);


////  /* USER CODE END 2 */

////  /* Infinite loop */
////  /* USER CODE BEGIN WHILE */

////// while (1)
//////  {
//////      uint32_t now_tick = HAL_GetTick();
//////      uint8_t pump_request = 0U;   /* pump_request（自动控制层给出的开泵请求） */
//////		
//////	  ESP_ServiceCmd_t service_cmd;
//////      memset(&service_cmd, 0, sizeof(service_cmd));

//////      /* 任务0：轮询 OneNET 服务调用
//////       * 说明：
//////       * 1. 收到 SetPump / SetLight / SetBeep 的 invoke 消息
//////       * 2. 先登记成 10 秒远程强制控制
//////       * 3. 再回复 invoke_reply
//////       */
//////      if (BSP_ESP8266_PollService(&service_cmd) != 0U)
//////      {
//////          APP_HandleServiceCommand(&service_cmd, now_tick);

//////          if (BSP_ESP8266_ReplyService(&service_cmd, 1U) != HAL_OK)
//////          {
//////              g_state.cloud_ok = 0U;
//////          }
//////      }
//////      /* =========================
//////       * 任务1：更新传感器数据
//////       * 说明：
//////       * 水泵运行期间暂停 I2C 相关访问，减少干扰
//////       * =========================
//////       */
//////      if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
//////      {
//////          g_sensor_tick = now_tick;

//////          if (g_i2c_pause == 0U)
//////          {
//////              APP_Sensor_Update(&g_sensor);
//////          }
//////      }

//////      /* =========================
//////       * 任务2：执行自动控制逻辑
//////       * 说明：
//////       * 这里先得到“期望状态”
//////       * =========================
//////       */
//////      APP_Control_Run(&g_sensor, &g_param, &g_state);
//////		
//////	  /* 云端服务控制优先级高于自动控制，但只保持一小段时间 */
//////      APP_ApplyRemoteOverrides(now_tick, &g_state);
//////	  
//////      /* 先保存自动控制层给出的开泵请求 */
//////      pump_request = g_state.pump_on;

//////      /* =========================
//////       * 任务3：水泵实际执行逻辑
//////       * 说明：
//////       * 1. 检测到土壤过干时，不直接无限开泵
//////       * 2. 采用“单次运行 + 冷却等待”的方式
//////       * 3. 开泵时暂停 I2C，停泵后恢复 OLED / BH1750
//////       * =========================
//////       */
//////      if (g_pump_running == 0U)
//////      {
//////          /* 当前未开泵 */
//////          g_state.pump_on = 0U;

//////          if ((pump_request != 0U) && (now_tick >= g_pump_lock_until))
//////          {
//////              /* 开泵前先提示 */
//////              OLED_ShowWatering();
//////              HAL_Delay(PUMP_PRESTART_MSG_MS);

//////              /* 开泵并暂停 I2C */
//////              g_i2c_pause = 1U;
//////              g_pump_running = 1U;
//////              g_pump_tick = HAL_GetTick();
//////              g_state.pump_on = 1U;
//////          }
//////      }
//////      else
//////      {
//////          /* 当前正在开泵 */
//////          g_state.pump_on = 1U;

//////          if ((now_tick - g_pump_tick) >= PUMP_RUN_MS)
//////          {
//////              /* 到时先关泵 */
//////              g_state.pump_on = 0U;
//////              BSP_Output_Update(&g_state);

//////              g_pump_running = 0U;

//////              /* 等待干扰衰减 */
//////              HAL_Delay(PUMP_RECOVER_DELAY_MS);

//////              /* 恢复 OLED / I2C */
//////              OLED_RecoverAfterPump();
//////              g_i2c_pause = 0U;

//////              /* 设置下次允许开泵的最早时刻 */
//////              g_pump_lock_until = HAL_GetTick() + PUMP_COOLDOWN_MS;

//////              /* 防止恢复后立刻被刷新覆盖 */
//////              g_display_tick = HAL_GetTick();
//////          }
//////      }

//////      /* =========================
//////       * 任务4：更新实际输出
//////       * 说明：
//////       * 1. PB0：水泵
//////       * 2. PB1：补光灯
//////       * 3. PB13：蜂鸣器（pH异常报警）
//////       * =========================
//////       */
//////      BSP_Output_Update(&g_state);

//////      /* =========================
//////       * 任务5：刷新 OLED 显示
//////       * 说明：
//////       * 固定显示传感器页，确保能看到土壤湿度 / 光照 / pH
//////       * =========================
//////       */
//////      if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
//////      {
//////          g_display_tick = now_tick;

//////          if (g_i2c_pause == 0U)
//////          {
//////              APP_Display_Show(PAGE_SENSOR, &g_sensor, &g_param, &g_state);
//////          }
//////      }
//////		
//////	   /* ========================================================
//////       * 任务6：OneNET 连接维护 + 定时上报属性
//////       * ======================================================== */
//////      if (g_state.cloud_ok == 0U)
//////      {
//////          if ((now_tick - g_onenet_retry_tick) >= ONENET_RETRY_PERIOD_MS)
//////          {
//////              g_onenet_retry_tick = now_tick;

//////              /* 【修改 1】：连接前，在屏幕第四行显示提示，化卡顿为合理交互 */
//////              BSP_OLED_ShowString(0, 6, "NET CONNECTING..");
//////              BSP_OLED_Refresh();

//////              if (BSP_ESP8266_StartOneNET() == HAL_OK)
//////              {
//////                  g_state.wifi_ok  = 1U;
//////                  g_state.cloud_ok = 1U;
//////                  /* 【修改 2】：连接成功，提示在线 */
//////                  BSP_OLED_ShowString(0, 6, "CLOUD ONLINE    "); 
//////              }
//////              else
//////              {
//////                  g_state.wifi_ok  = 0U;
//////                  g_state.cloud_ok = 0U;
//////                  /* 【修改 3】：连接失败，提示离线，并延长重试时间到20秒，留出时间演示本地功能 */
//////                  BSP_OLED_ShowString(0, 6, "CLOUD FAILED    "); 
//////                  g_onenet_retry_tick = now_tick + 10000U; // 额外增加10秒惩罚等待，避免频繁卡顿
//////              }
//////              BSP_OLED_Refresh();
//////              
//////              /* 防止连网太久导致后续的传感器刷新瞬间乱套，重置一下时间戳 */
//////              g_display_tick = HAL_GetTick();
//////              g_sensor_tick = HAL_GetTick();
//////          }
//////      }
//////      else
//////      {
//////          /* 已连接时，定时上报属性... (保留您原来的代码不变) */
//////          if ((now_tick - g_cloud_tick) >= CLOUD_UPLOAD_PERIOD_MS)
//////          // ... 后面不动 ...
//////          {
//////              g_cloud_tick = now_tick;

////////              APP_Cloud_Upload(&g_sensor, &g_state);

////////              if (BSP_ESP8266_PublishPropertyJson(APP_Cloud_GetLastPacket()) == HAL_OK)
////////              {
////////                  g_state.cloud_ok = 1U;
////////              }
////////              else
////////              {
////////                  g_state.cloud_ok = 0U;
////////              }
//////          }
//////      }
//////      HAL_Delay(10);
//////  }


///////* Infinite loop */
//////  /* USER CODE BEGIN WHILE */
//////  while (1)
//////  {
////////	  /* 临时调试代码：强制拉高所有控制引脚 */
////////HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // 水泵
////////HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET); // 补光灯
////////HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // 蜂鸣器(低电平触发)
//////      uint32_t now_tick = HAL_GetTick();
//////      uint8_t pump_request = 0U;
//////      ESP_ServiceCmd_t service_cmd;
//////      memset(&service_cmd, 0, sizeof(service_cmd));

//////      /* ========================================================
//////       * 任务0：轮询 OneNET 服务调用
//////       * ======================================================== */
//////      if (BSP_ESP8266_PollService(&service_cmd) != 0U)
//////      {
//////          /* 1. 收到云端指令（SetLight / SetPump 等） */
//////          APP_HandleServiceCommand(&service_cmd, now_tick);

//////          /* 2. 立即回复云端，防止云端显示“响应超时” */
//////          if (BSP_ESP8266_ReplyService(&service_cmd, 1U) != HAL_OK)
//////          {
//////              g_state.cloud_ok = 0U;
//////          }
//////      }

//////      /* ========================================================
//////       * 任务1：更新传感器数据
//////       * ======================================================== */
//////      if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
//////      {
//////          g_sensor_tick = now_tick;
//////          if (g_i2c_pause == 0U)
//////          {
//////              APP_Sensor_Update(&g_sensor);
//////          }
//////      }

//////      /* ========================================================
//////       * 任务2：执行逻辑判断（重点修改区）
//////       * ======================================================== */
//////      
//////      /* A. 先运行本地自动控制逻辑（基于传感器数据判断是否该开灯/开泵） */
//////      APP_Control_Run(&g_sensor, &g_param, &g_state);

//////      /* B. 【关键】应用远程强制覆盖。如果10秒内有云端指令，它会覆盖上面的本地判断结果 */
//////      APP_ApplyRemoteOverrides(now_tick, &g_state);

//////      /* C. 将最终确定的期望状态存入请求变量 */
//////      pump_request = g_state.pump_on;
//////      // 补光灯和蜂鸣器不需要额外状态机，直接使用 g_state.light_on 和 g_state.beep_on 即可

//////      /* ========================================================
//////       * 任务3：水泵实际执行状态机（单次运行 + 冷却模式）
//////       * ======================================================== */
//////      if (g_pump_running == 0U)
//////      {
//////          /* 只有在满足：有开启请求 且 已经过了冷却时间 才会启动 */
//////          if ((pump_request != 0U) && (now_tick >= g_pump_lock_until))
//////          {
//////              OLED_ShowWatering();
//////              HAL_Delay(PUMP_PRESTART_MSG_MS);

//////              g_i2c_pause = 1U;
//////              g_pump_running = 1U;
//////              g_pump_tick = HAL_GetTick();
//////              g_state.pump_on = 1U;
//////          }
//////          else
//////          {
//////              /* 确保在没有请求时，状态位是准确的 0 */
//////              g_state.pump_on = 0U;
//////          }
//////      }
//////      else
//////      {
//////          /* 正在浇水，强制保持开启状态直到时间结束 */
//////          g_state.pump_on = 1U;

//////          if ((now_tick - g_pump_tick) >= PUMP_RUN_MS)
//////          {
//////              g_state.pump_on = 0U;
//////              /* 立即更新硬件，确保水泵瞬间停止 */
//////              BSP_Output_Update(&g_state);

//////              g_pump_running = 0U;
//////              HAL_Delay(PUMP_RECOVER_DELAY_MS);

//////              OLED_RecoverAfterPump();
//////              g_i2c_pause = 0U;
//////              g_pump_lock_until = HAL_GetTick() + PUMP_COOLDOWN_MS;
//////              g_display_tick = HAL_GetTick();
//////          }
//////      }

//////      /* ========================================================
//////       * 任务4：统一更新物理输出（驱动 PB0, PB1, PB13）
//////       * ======================================================== */
//////      /* 此时 g_state 里的 light_on 和 pump_on 已经是经过覆盖且未被清零的最终值 */
//////      BSP_Output_Update(&g_state);

//////      /* ========================================================
//////       * 任务5：刷新 OLED 显示
//////       * ======================================================== */
//////      if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
//////      {
//////          g_display_tick = now_tick;
//////          if (g_i2c_pause == 0U)
//////          {
//////              APP_Display_Show(PAGE_SENSOR, &g_sensor, &g_param, &g_state);
//////          }
//////      }

//////      /* ========================================================
//////       * 任务6：OneNET 连接维护
//////       * ======================================================== */
//////      if (g_state.cloud_ok == 0U)
//////      {
//////          if ((now_tick - g_onenet_retry_tick) >= ONENET_RETRY_PERIOD_MS)
//////          {
//////              g_onenet_retry_tick = now_tick;
//////              BSP_OLED_ShowString(0, 6, "NET CONNECTING..");
//////              BSP_OLED_Refresh();

//////              if (BSP_ESP8266_StartOneNET() == HAL_OK)
//////              {
//////                  g_state.wifi_ok = 1U;
//////                  g_state.cloud_ok = 1U;
//////                  BSP_OLED_ShowString(0, 6, "CLOUD ONLINE    ");
//////              }
//////              else
//////              {
//////                  g_state.wifi_ok = 0U;
//////                  g_state.cloud_ok = 0U;
//////                  BSP_OLED_ShowString(0, 6, "CLOUD FAILED    ");
//////                  g_onenet_retry_tick = now_tick + 10000U;
//////              }
//////              BSP_OLED_Refresh();
//////              g_display_tick = HAL_GetTick();
//////              g_sensor_tick = HAL_GetTick();
//////          }
//////      }
//////      else
//////      {
//////          /* 定时上报逻辑... */
//////          if ((now_tick - g_cloud_tick) >= CLOUD_UPLOAD_PERIOD_MS)
//////          {
//////              g_cloud_tick = now_tick;
//////              // APP_Cloud_Upload 逻辑...
//////          }
//////      }
//////      HAL_Delay(10);
//////  }
////  
////  while (1)
////  {
////      uint32_t now_tick = HAL_GetTick();
////      ESP_ServiceCmd_t service_cmd;
////      memset(&service_cmd, 0, sizeof(service_cmd));

////      /* 任务0：接收云端指令 */
////      if (BSP_ESP8266_PollService(&service_cmd) != 0U)
////      {
////          APP_HandleServiceCommand(&service_cmd, now_tick);
////          (void)BSP_ESP8266_ReplyService(&service_cmd, 1U);
////      }

////      /* 任务1：传感器更新 */
////      if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
////      {
////          g_sensor_tick = now_tick;
////          if (g_i2c_pause == 0U) { APP_Sensor_Update(&g_sensor); }
////      }

////      /* ========================================================
////       * 任务2：控制逻辑判断 (核心修复：使用临时变量锁定请求)
////       * ======================================================== */
////      
////      // A. 先运行自动逻辑，允许它读写 g_state
////      APP_Control_Run(&g_sensor, &g_param, &g_state);

////      // B. 运行远程覆盖，如果 10s 内有指令，它会覆盖 g_state.pump_on
////      APP_ApplyRemoteOverrides(now_tick, &g_state);

////      // C. 【关键修复】：用一个局部变量把这一轮最终的“开泵意愿”锁死
////      uint8_t final_pump_intent = g_state.pump_on;

////      /* ========================================================
////       * 任务3：水泵执行状态机 (解决冷却和干涉)
////       * ======================================================== */
////      if (g_pump_running == 0U)
////      {
////          // 只有在这里才真正判断是否满足开泵条件
////          if ((final_pump_intent != 0U) && (now_tick >= g_pump_lock_until))
////          {
////              OLED_ShowWatering();
////              HAL_Delay(PUMP_PRESTART_MSG_MS);

////              g_i2c_pause = 1U;
////              g_pump_running = 1U;
////              g_pump_tick = now_tick;
////              g_state.pump_on = 1U; // 真正开启状态
////          }
////          else
////          {
////              g_state.pump_on = 0U; // 确保不满足条件时引脚是灭的
////          }
////      }
////      else
////      {
////          g_state.pump_on = 1U; // 强制锁定开启状态
////          if ((now_tick - g_pump_tick) >= PUMP_RUN_MS)
////          {
////              g_state.pump_on = 0U;
////              BSP_Output_Update(&g_state); // 立即物理关闭
////              g_pump_running = 0U;
////              HAL_Delay(PUMP_RECOVER_DELAY_MS);
////              OLED_RecoverAfterPump();
////              g_i2c_pause = 0U;
////              g_pump_lock_until = HAL_GetTick() + PUMP_COOLDOWN_MS;
////          }
////      }

////      /* ========================================================
////       * 任务4：物理输出 (这是唯一一处真正操作 GPIO 的地方)
////       * ======================================================== */
////      BSP_Output_Update(&g_state);

////      /* 任务5：显示 */
////      if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
////      {
////          g_display_tick = now_tick;
////          if (g_i2c_pause == 0U) { APP_Display_Show(PAGE_SENSOR, &g_sensor, &g_param, &g_state); }
////      }

////      /* 任务6：网络连接维护 (代码略) */
////      // ... 保持你之前的重连逻辑即可 ...
////	 //* 任务6：OneNET 连接维护 + 定时上报属性
//////       * ======================================================== */
////      if (g_state.cloud_ok == 0U)
////      {
////          if ((now_tick - g_onenet_retry_tick) >= ONENET_RETRY_PERIOD_MS)
////          {
////              g_onenet_retry_tick = now_tick;

////              /* 【修改 1】：连接前，在屏幕第四行显示提示，化卡顿为合理交互 */
////              BSP_OLED_ShowString(0, 6, "NET CONNECTING..");
////              BSP_OLED_Refresh();

////              if (BSP_ESP8266_StartOneNET() == HAL_OK)
////              {
////                  g_state.wifi_ok  = 1U;
////                  g_state.cloud_ok = 1U;
////                  /* 【修改 2】：连接成功，提示在线 */
////                  BSP_OLED_ShowString(0, 6, "CLOUD ONLINE    "); 
////              }
////              else
////              {
////                  g_state.wifi_ok  = 0U;
////                  g_state.cloud_ok = 0U;
////                  /* 【修改 3】：连接失败，提示离线，并延长重试时间到20秒，留出时间演示本地功能 */
////                  BSP_OLED_ShowString(0, 6, "CLOUD FAILED    "); 
////                  g_onenet_retry_tick = now_tick + 10000U; // 额外增加10秒惩罚等待，避免频繁卡顿
////              }
////              BSP_OLED_Refresh();
////              
////              /* 防止连网太久导致后续的传感器刷新瞬间乱套，重置一下时间戳 */
////              g_display_tick = HAL_GetTick();
////              g_sensor_tick = HAL_GetTick();
////          }
////      }
////      else
////      {
////          /* 已连接时，定时上报属性... (保留您原来的代码不变) */
////          if ((now_tick - g_cloud_tick) >= CLOUD_UPLOAD_PERIOD_MS)
////          // ... 后面不动 ...
////          {
////              g_cloud_tick = now_tick;

//////              APP_Cloud_Upload(&g_sensor, &g_state);

//////              if (BSP_ESP8266_PublishPropertyJson(APP_Cloud_GetLastPacket()) == HAL_OK)
//////              {
//////                  g_state.cloud_ok = 1U;
//////              }
//////              else
//////              {
//////                  g_state.cloud_ok = 0U;
//////              }
////          }
////      }
////      HAL_Delay(10);
////  }

////}

////    /* USER CODE END WHILE */

////    /* USER CODE BEGIN 3 */
////  /* USER CODE END 3 */


/////**
////  * @brief System Clock Configuration
////  * @retval None
////  */
////void SystemClock_Config(void)
////{
////  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
////  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
////  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

////  /** Initializes the RCC Oscillators according to the specified parameters
////  * in the RCC_OscInitTypeDef structure.
////  */
////  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
////  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
////  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
////  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
////  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
////  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
////  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
////  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
////  {
////    Error_Handler();
////  }

////  /** Initializes the CPU, AHB and APB buses clocks
////  */
////  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
////                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
////  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
////  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
////  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
////  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

////  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
////  {
////    Error_Handler();
////  }
////  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
////  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
////  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
////  {
////    Error_Handler();
////  }
////}

/////* USER CODE BEGIN 4 */

/////* USER CODE END 4 */

/////**
////  * @brief  This function is executed in case of error occurrence.
////  * @retval None
////  */
////void Error_Handler(void)
////{
////  /* USER CODE BEGIN Error_Handler_Debug */
//////  __disable_irq();
//////  while (1)
//////  {
//////  }
////  /* USER CODE END Error_Handler_Debug */
////}
////#ifdef USE_FULL_ASSERT
/////**
////  * @brief  Reports the name of the source file and the source line number
////  *         where the assert_param error has occurred.
////  * @param  file: pointer to the source file name
////  * @param  line: assert_param error line source number
////  * @retval None
////  */
////void assert_failed(uint8_t *file, uint32_t line)
////{
////  /* USER CODE BEGIN 6 */
//////  /* 这里可根据需要加入调试打印 */
////  /* USER CODE END 6 */
////}
////#endif /* USE_FULL_ASSERT */


///* USER CODE BEGIN Header */
/////**
////  ******************************************************************************
////  * @file           : main.c
////  * @brief          : Main program body
////  ******************************************************************************
////  * @attention
////  *
////  * Copyright (c) 2026 STMicroelectronics.
////  * All rights reserved.
////  *
////  * This software is licensed under terms that can be found in the LICENSE file
////  * in the root directory of this software component.
////  * If no LICENSE file comes with this software, it is provided AS-IS.
////  *
////  ******************************************************************************
////  */
///* USER CODE BEGIN Header */
/////**
////  ******************************************************************************
////  * @file           : main.c
////  * @brief          : Main program body
////  ******************************************************************************
////  * @attention
////  *
////  * Copyright (c) 2026 STMicroelectronics.
////  * All rights reserved.
////  *
////  * This software is licensed under terms that can be found in the LICENSE file
////  * in the root directory of this software component.
////  * If no LICENSE file comes with this software, it is provided AS-IS.
////  *
////  ******************************************************************************
////  */
///* USER CODE BEGIN Header */
/////**
////  ******************************************************************************
////  * @file           : main.c
////  * @brief          : Main program body
////  ******************************************************************************
////  * @attention
////  *
////  * Copyright (c) 2026 STMicroelectronics.
////  * All rights reserved.
////  *
////  * This software is licensed under terms that can be found in the LICENSE file
////  * in the root directory of this software component.
////  * If no LICENSE file comes with this software, it is provided AS-IS.
////  *
////  ******************************************************************************
////  */
///* USER CODE BEGIN Header */
/////**
////  ******************************************************************************
////  * @file           : main.c
////  * @brief          : Main program body
////  ******************************************************************************
////  * @attention
////  *
////  * Copyright (c) 2026 STMicroelectronics.
////  * All rights reserved.
////  *
////  * This software is licensed under terms that can be found in the LICENSE file
////  * in the root directory of this software component.
////  * If no LICENSE file comes with this software, it is provided AS-IS.
////  *
////  ******************************************************************************
////  */
///* USER CODE END Header */
///* Includes ------------------------------------------------------------------*/
//#include "main.h"
//#include "adc.h"
//#include "i2c.h"
//#include "tim.h"
//#include "usart.h"
//#include "gpio.h"

///* Private includes ----------------------------------------------------------*/
///* USER CODE BEGIN Includes */
//#include "plant_types.h"
//#include "plant_config.h"
//#include "app_sensor.h"
//#include "app_control.h"
//#include "app_display.h"
//#include "app_menu.h"
//#include "app_cloud.h"
//#include "bsp_oled.h"
//#include "bsp_output.h"
//#include "app_ph.h"
//#include "bsp_bh1750.h"
//#include "bsp_dht11.h"
//#include "bsp_soil.h"
//#include "bsp_esp8266.h"
//#include <string.h>

///* USER CODE END Includes */

///* Private typedef -----------------------------------------------------------*/
///* USER CODE BEGIN PTD */
//typedef struct
//{
//    uint8_t active;     /* 是否处于远程强制控制期 */
//    uint8_t value;      /* 强制值 0/1 */
//    uint32_t until;     /* 到期时间戳 */
//} RemoteOverride_t;
///* USER CODE END PTD */

///* Private define ------------------------------------------------------------*/
///* USER CODE BEGIN PD */

///* 水泵单次运行时间
// * 说明：检测到土壤过干后，先运行固定时间，再停泵观察
// */
//#define PUMP_RUN_MS             (2000U)

///* 水泵启动前提示保留时间 */
//#define PUMP_PRESTART_MSG_MS    (800U)

///* 水泵停止后恢复等待时间 */
//#define PUMP_RECOVER_DELAY_MS   (500U)

///* 两次浇水动作之间的最短等待时间
// * 说明：避免水泵频繁启停
// */
//#define PUMP_COOLDOWN_MS        (15000U)

///* 当前整机参数
// * 说明：
// * 1. 土壤湿度当前仍用 ADC 原始值，数值越大越干
// * 2. 光照阈值按你室内实测先设为 100 lux
// * 3. pH 报警范围当前先设得宽一点，避免误报
// */
//#define SOIL_DRY_TH_RAW         (4200.0f)
//#define LIGHT_ON_TH_LUX         (100.0f)
//#define PH_ALARM_LOW_TH         (5.0f)
//#define PH_ALARM_HIGH_TH        (7.5f)

//#define ONENET_RETRY_PERIOD_MS   (10000U)   /* OneNET 重连周期 10 秒 */

//#define CLOUD_UPLOAD_PERIOD_MS    (15000U)   /* CLOUD_UPLOAD_PERIOD_MS（上云周期），当前设为5秒 */
//#define REMOTE_OVERRIDE_MS      (10000U)   /* 云端服务控制保持 10 秒 */
///* USER CODE END PD */

///* Private macro -------------------------------------------------------------*/
///* USER CODE BEGIN PM */

///* USER CODE END PM */

///* Private variables ---------------------------------------------------------*/

///* USER CODE BEGIN PV */

//SensorData_t  g_sensor;        /* g_sensor（实时传感器数据） */
//SystemParam_t g_param;         /* g_param（系统参数） */
//SystemState_t g_state;         /* g_state（系统状态） */

//uint32_t g_sensor_tick  = 0U;  /* g_sensor_tick（传感器调度时间戳） */
//uint32_t g_display_tick = 0U;  /* g_display_tick（显示调度时间戳） */

//uint8_t  g_i2c_pause = 0U;          /* g_i2c_pause（I2C暂停标志） */
//uint8_t  g_pump_running = 0U;       /* g_pump_running（水泵实际运行标志） */
//uint32_t g_pump_tick = 0U;          /* g_pump_tick（水泵启动时间戳） */
//uint32_t g_pump_lock_until = 0U;    /* g_pump_lock_until（下次允许开泵的最早时刻） */
//uint32_t g_cloud_tick = 0U;   /* g_cloud_tick（云端上传调度时间戳） */
//RemoteOverride_t g_remote_pump;
//RemoteOverride_t g_remote_light;
//RemoteOverride_t g_remote_beep;

//uint32_t g_onenet_retry_tick = 0U;   /* OneNET 重连调度时间戳 */
///* USER CODE END PV */

///* Private function prototypes -----------------------------------------------*/
//void SystemClock_Config(void);
///* USER CODE BEGIN PFP */

//static void APP_DefaultParam_Init(SystemParam_t *param);
//static void OLED_ShowReady(void);
//static void OLED_ShowWatering(void);
//static void OLED_I2C_BusRecover(void);
//static void OLED_RecoverAfterPump(void);

//static void APP_ClearRemoteOverrides(void);
//static void APP_ApplyRemoteOverrides(uint32_t now_tick, SystemState_t *state);
//static void APP_HandleServiceCommand(const ESP_ServiceCmd_t *cmd, uint32_t now_tick);

///* USER CODE END PFP */

///* Private user code ---------------------------------------------------------*/
///* USER CODE BEGIN 0 */

///* OLED_ShowReady（显示系统就绪界面） */
//static void OLED_ShowReady(void)
//{
//    BSP_OLED_Clear();
//    BSP_OLED_ShowString(0, 0, "PLANT SYS");
//    BSP_OLED_ShowString(0, 2, "READY");
//    BSP_OLED_Refresh();
//}

///* OLED_ShowWatering（显示浇水提示界面） */
//static void OLED_ShowWatering(void)
//{
//    BSP_OLED_Clear();
//    BSP_OLED_ShowString(0, 0, "PLANT SYS");
//    BSP_OLED_ShowString(0, 2, "WATERING...");
//    BSP_OLED_Refresh();
//}

///* OLED_I2C_BusRecover（I2C总线恢复） */
//static void OLED_I2C_BusRecover(void)
//{
//    GPIO_InitTypeDef GPIO_InitStruct = {0};
//    uint8_t i = 0U;

//    HAL_I2C_DeInit(&hi2c1);

//    __HAL_RCC_GPIOB_CLK_ENABLE();

//    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
//    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
//    GPIO_InitStruct.Pull = GPIO_PULLUP;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//    /* 释放总线 */
//    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
//    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
//    HAL_Delay(1);

//    /* SCL 输出 9 个时钟脉冲 */
//    for (i = 0U; i < 9U; i++)
//    {
//        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
//        HAL_Delay(1);
//        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
//        HAL_Delay(1);
//    }

//    /* 产生一次 STOP 条件 */
//    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
//    HAL_Delay(1);
//    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
//    HAL_Delay(1);
//    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
//    HAL_Delay(1);

//    MX_I2C1_Init();
//}

///* OLED_RecoverAfterPump（停泵后恢复 OLED 显示） */
//static void OLED_RecoverAfterPump(void)
//{
//    OLED_I2C_BusRecover();

//    HAL_Delay(10);

//    BSP_OLED_Init();
//    BSP_OLED_Clear();
//    BSP_OLED_ShowString(0, 0, "PLANT SYS");
//    BSP_OLED_ShowString(0, 2, "PUMP DONE");
//    BSP_OLED_Refresh();

//    HAL_Delay(1000);

//    OLED_ShowReady();
//}

///* APP_DefaultParam_Init（默认参数初始化） */
//static void APP_DefaultParam_Init(SystemParam_t *param)
//{
//    if (param == 0)
//    {
//        return;
//    }

//    param->soil_low_th   = DEF_SOIL_LOW_TH;
//    param->light_low_th  = DEF_LIGHT_LOW_TH;
//    param->ph_low_th     = DEF_PH_LOW_TH;
//    param->ph_high_th    = DEF_PH_HIGH_TH;
//    param->auto_water_en = 1U;
//    param->auto_light_en = 1U;
//    param->alarm_en      = 1U;
//}

///* APP_ClearRemoteOverrides（清空远程强制控制） */
//static void APP_ClearRemoteOverrides(void)
//{
//    memset(&g_remote_pump,  0, sizeof(g_remote_pump));
//    memset(&g_remote_light, 0, sizeof(g_remote_light));
//    memset(&g_remote_beep,  0, sizeof(g_remote_beep));
//}

///* APP_HandleServiceCommand（处理 OneNET 服务调用）
// * 说明：
// * 1. 收到服务调用后，不直接永久改自动控制逻辑
// * 2. 只做一个 10 秒的远程强制控制窗口
// * 3. 到期后系统重新回到自动控制
// */
//static void APP_HandleServiceCommand(const ESP_ServiceCmd_t *cmd, uint32_t now_tick)
//{
//    if (cmd == 0)
//    {
//        return;
//    }

//    switch (cmd->type)
//    {
//        case ESP_SERVICE_SET_PUMP:
//            g_remote_pump.active = 1U;
//            g_remote_pump.value  = cmd->value;
//            g_remote_pump.until  = now_tick + REMOTE_OVERRIDE_MS;
//            break;

//        case ESP_SERVICE_SET_LIGHT:
//            g_remote_light.active = 1U;
//            g_remote_light.value  = cmd->value;
//            g_remote_light.until  = now_tick + REMOTE_OVERRIDE_MS;
//            if (cmd->value == 1U)
//            {
//                BSP_OLED_ShowString(0, 4, "LIGHT CMD: ON ");
//            }
//            else
//            {
//                BSP_OLED_ShowString(0, 4, "LIGHT CMD: OFF");
//            }
//            BSP_OLED_Refresh();
//            break;

//        case ESP_SERVICE_SET_BEEP:
//            g_remote_beep.active = 1U;
//            g_remote_beep.value  = cmd->value;
//            g_remote_beep.until  = now_tick + REMOTE_OVERRIDE_MS;
//            break;

//        default:
//            break;
//    }
//}



///* APP_ApplyRemoteOverrides（应用远程强制控制） */
//static void APP_ApplyRemoteOverrides(uint32_t now_tick, SystemState_t *state)
//{
//    if (state == 0)
//    {
//        return;
//    }

//    if ((g_remote_pump.active != 0U) && (now_tick < g_remote_pump.until))
//    {
//        state->pump_on = g_remote_pump.value;
//    }
//    else
//    {
//        g_remote_pump.active = 0U;
//    }

//    if ((g_remote_light.active != 0U) && (now_tick < g_remote_light.until))
//    {
//        state->light_on = g_remote_light.value;
//    }
//    else
//    {
//        g_remote_light.active = 0U;
//    }

//    if ((g_remote_beep.active != 0U) && (now_tick < g_remote_beep.until))
//    {
//        state->beep_on = g_remote_beep.value;
//    }
//    else
//    {
//        g_remote_beep.active = 0U;
//    }
//}

///* USER CODE END 0 */

///**
//  * @brief  The application entry point.
//  * @retval int
//  */
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

// /* =========================
//   * 底层模块初始化
//   * =========================
//   */

//  BSP_OLED_Init();
//  BSP_BH1750_Init();
//  BSP_DHT11_Init();
//  BSP_Soil_Init();
//  /* 注意：USART1 已经给 ESP8266 用，不能再初始化调试串口输出 */
//  /* =========================
//   * 上层参数与状态初始化
//   * =========================
//   */
//  APP_DefaultParam_Init(&g_param);
//  APP_Control_Init(&g_state);
//  APP_Menu_Init();
//  APP_Display_Init();
//  APP_Sensor_Init(&g_sensor);
//  BSP_Output_Init();
//  APP_PH_Init();
//  
//	  /* 云端与 ESP8266 初始化 */
//	APP_Cloud_Init(&g_state);
//	BSP_ESP8266_Init();
//	APP_ClearRemoteOverrides();

//	g_cloud_tick = HAL_GetTick();
//	g_onenet_retry_tick = HAL_GetTick();

//	g_state.wifi_ok  = 0U;
//	g_state.cloud_ok = 0U;
//  
//  g_sensor_tick  = HAL_GetTick();
//  g_display_tick = HAL_GetTick();

//	OLED_ShowReady();
//	HAL_Delay(300);
//  /* 关闭 PC13 板载 LED */
//  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

//  /* =========================
//   * 当前整机工作参数
//   * =========================
//   */
//  g_param.soil_low_th   = SOIL_DRY_TH_RAW;     /* 土壤干燥阈值（ADC原始值） */
//  g_param.light_low_th  = LIGHT_ON_TH_LUX;     /* 自动补光开启阈值 */
//  g_param.ph_low_th     = PH_ALARM_LOW_TH;     /* pH 报警下限 */
//  g_param.ph_high_th    = PH_ALARM_HIGH_TH;    /* pH 报警上限 */
//  g_param.auto_water_en = 1U;
//  g_param.auto_light_en = 0U;
//  g_param.alarm_en      = 1U;

//  /* 初始状态 */
//  g_state.pump_on  = 0U;
//  g_state.light_on = 0U;
//  g_state.beep_on  = 0U;

//  g_i2c_pause      = 0U;
//  g_pump_running   = 0U;
//  g_pump_tick      = 0U;
//  g_pump_lock_until = HAL_GetTick();

////  OLED_ShowReady();
////  HAL_Delay(1000);


//  /* USER CODE END 2 */

//  /* Infinite loop */
//  /* USER CODE BEGIN WHILE */

//// while (1)
////  {
////      uint32_t now_tick = HAL_GetTick();
////      uint8_t pump_request = 0U;   /* pump_request（自动控制层给出的开泵请求） */
////		
////	  ESP_ServiceCmd_t service_cmd;
////      memset(&service_cmd, 0, sizeof(service_cmd));

////      /* 任务0：轮询 OneNET 服务调用
////       * 说明：
////       * 1. 收到 SetPump / SetLight / SetBeep 的 invoke 消息
////       * 2. 先登记成 10 秒远程强制控制
////       * 3. 再回复 invoke_reply
////       */
////      if (BSP_ESP8266_PollService(&service_cmd) != 0U)
////      {
////          APP_HandleServiceCommand(&service_cmd, now_tick);

////          if (BSP_ESP8266_ReplyService(&service_cmd, 1U) != HAL_OK)
////          {
////              g_state.cloud_ok = 0U;
////          }
////      }
////      /* =========================
////       * 任务1：更新传感器数据
////       * 说明：
////       * 水泵运行期间暂停 I2C 相关访问，减少干扰
////       * =========================
////       */
////      if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
////      {
////          g_sensor_tick = now_tick;

////          if (g_i2c_pause == 0U)
////          {
////              APP_Sensor_Update(&g_sensor);
////          }
////      }

////      /* =========================
////       * 任务2：执行自动控制逻辑
////       * 说明：
////       * 这里先得到“期望状态”
////       * =========================
////       */
////      APP_Control_Run(&g_sensor, &g_param, &g_state);
////		
////	  /* 云端服务控制优先级高于自动控制，但只保持一小段时间 */
////      APP_ApplyRemoteOverrides(now_tick, &g_state);
////	  
////      /* 先保存自动控制层给出的开泵请求 */
////      pump_request = g_state.pump_on;

////      /* =========================
////       * 任务3：水泵实际执行逻辑
////       * 说明：
////       * 1. 检测到土壤过干时，不直接无限开泵
////       * 2. 采用“单次运行 + 冷却等待”的方式
////       * 3. 开泵时暂停 I2C，停泵后恢复 OLED / BH1750
////       * =========================
////       */
////      if (g_pump_running == 0U)
////      {
////          /* 当前未开泵 */
////          g_state.pump_on = 0U;

////          if ((pump_request != 0U) && (now_tick >= g_pump_lock_until))
////          {
////              /* 开泵前先提示 */
////              OLED_ShowWatering();
////              HAL_Delay(PUMP_PRESTART_MSG_MS);

////              /* 开泵并暂停 I2C */
////              g_i2c_pause = 1U;
////              g_pump_running = 1U;
////              g_pump_tick = HAL_GetTick();
////              g_state.pump_on = 1U;
////          }
////      }
////      else
////      {
////          /* 当前正在开泵 */
////          g_state.pump_on = 1U;

////          if ((now_tick - g_pump_tick) >= PUMP_RUN_MS)
////          {
////              /* 到时先关泵 */
////              g_state.pump_on = 0U;
////              BSP_Output_Update(&g_state);

////              g_pump_running = 0U;

////              /* 等待干扰衰减 */
////              HAL_Delay(PUMP_RECOVER_DELAY_MS);

////              /* 恢复 OLED / I2C */
////              OLED_RecoverAfterPump();
////              g_i2c_pause = 0U;

////              /* 设置下次允许开泵的最早时刻 */
////              g_pump_lock_until = HAL_GetTick() + PUMP_COOLDOWN_MS;

////              /* 防止恢复后立刻被刷新覆盖 */
////              g_display_tick = HAL_GetTick();
////          }
////      }

////      /* =========================
////       * 任务4：更新实际输出
////       * 说明：
////       * 1. PB0：水泵
////       * 2. PB1：补光灯
////       * 3. PB13：蜂鸣器（pH异常报警）
////       * =========================
////       */
////      BSP_Output_Update(&g_state);

////      /* =========================
////       * 任务5：刷新 OLED 显示
////       * 说明：
////       * 固定显示传感器页，确保能看到土壤湿度 / 光照 / pH
////       * =========================
////       */
////      if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
////      {
////          g_display_tick = now_tick;

////          if (g_i2c_pause == 0U)
////          {
////              APP_Display_Show(PAGE_SENSOR, &g_sensor, &g_param, &g_state);
////          }
////      }
////		
////	   /* ========================================================
////       * 任务6：OneNET 连接维护 + 定时上报属性
////       * ======================================================== */
////      if (g_state.cloud_ok == 0U)
////      {
////          if ((now_tick - g_onenet_retry_tick) >= ONENET_RETRY_PERIOD_MS)
////          {
////              g_onenet_retry_tick = now_tick;

////              /* 【修改 1】：连接前，在屏幕第四行显示提示，化卡顿为合理交互 */
////              BSP_OLED_ShowString(0, 6, "NET CONNECTING..");
////              BSP_OLED_Refresh();

////              if (BSP_ESP8266_StartOneNET() == HAL_OK)
////              {
////                  g_state.wifi_ok  = 1U;
////                  g_state.cloud_ok = 1U;
////                  /* 【修改 2】：连接成功，提示在线 */
////                  BSP_OLED_ShowString(0, 6, "CLOUD ONLINE    "); 
////              }
////              else
////              {
////                  g_state.wifi_ok  = 0U;
////                  g_state.cloud_ok = 0U;
////                  /* 【修改 3】：连接失败，提示离线，并延长重试时间到20秒，留出时间演示本地功能 */
////                  BSP_OLED_ShowString(0, 6, "CLOUD FAILED    "); 
////                  g_onenet_retry_tick = now_tick + 10000U; // 额外增加10秒惩罚等待，避免频繁卡顿
////              }
////              BSP_OLED_Refresh();
////              
////              /* 防止连网太久导致后续的传感器刷新瞬间乱套，重置一下时间戳 */
////              g_display_tick = HAL_GetTick();
////              g_sensor_tick = HAL_GetTick();
////          }
////      }
////      else
////      {
////          /* 已连接时，定时上报属性... (保留您原来的代码不变) */
////          if ((now_tick - g_cloud_tick) >= CLOUD_UPLOAD_PERIOD_MS)
////          // ... 后面不动 ...
////          {
////              g_cloud_tick = now_tick;

//////              APP_Cloud_Upload(&g_sensor, &g_state);

//////              if (BSP_ESP8266_PublishPropertyJson(APP_Cloud_GetLastPacket()) == HAL_OK)
//////              {
//////                  g_state.cloud_ok = 1U;
//////              }
//////              else
//////              {
//////                  g_state.cloud_ok = 0U;
//////              }
////          }
////      }
////      HAL_Delay(10);
////  }


/////* Infinite loop */
////  /* USER CODE BEGIN WHILE */
////  while (1)
////  {
//////	  /* 临时调试代码：强制拉高所有控制引脚 */
//////HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // 水泵
//////HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET); // 补光灯
//////HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // 蜂鸣器(低电平触发)
////      uint32_t now_tick = HAL_GetTick();
////      uint8_t pump_request = 0U;
////      ESP_ServiceCmd_t service_cmd;
////      memset(&service_cmd, 0, sizeof(service_cmd));

////      /* ========================================================
////       * 任务0：轮询 OneNET 服务调用
////       * ======================================================== */
////      if (BSP_ESP8266_PollService(&service_cmd) != 0U)
////      {
////          /* 1. 收到云端指令（SetLight / SetPump 等） */
////          APP_HandleServiceCommand(&service_cmd, now_tick);

////          /* 2. 立即回复云端，防止云端显示“响应超时” */
////          if (BSP_ESP8266_ReplyService(&service_cmd, 1U) != HAL_OK)
////          {
////              g_state.cloud_ok = 0U;
////          }
////      }

////      /* ========================================================
////       * 任务1：更新传感器数据
////       * ======================================================== */
////      if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
////      {
////          g_sensor_tick = now_tick;
////          if (g_i2c_pause == 0U)
////          {
////              APP_Sensor_Update(&g_sensor);
////          }
////      }

////      /* ========================================================
////       * 任务2：执行逻辑判断（重点修改区）
////       * ======================================================== */
////      
////      /* A. 先运行本地自动控制逻辑（基于传感器数据判断是否该开灯/开泵） */
////      APP_Control_Run(&g_sensor, &g_param, &g_state);

////      /* B. 【关键】应用远程强制覆盖。如果10秒内有云端指令，它会覆盖上面的本地判断结果 */
////      APP_ApplyRemoteOverrides(now_tick, &g_state);

////      /* C. 将最终确定的期望状态存入请求变量 */
////      pump_request = g_state.pump_on;
////      // 补光灯和蜂鸣器不需要额外状态机，直接使用 g_state.light_on 和 g_state.beep_on 即可

////      /* ========================================================
////       * 任务3：水泵实际执行状态机（单次运行 + 冷却模式）
////       * ======================================================== */
////      if (g_pump_running == 0U)
////      {
////          /* 只有在满足：有开启请求 且 已经过了冷却时间 才会启动 */
////          if ((pump_request != 0U) && (now_tick >= g_pump_lock_until))
////          {
////              OLED_ShowWatering();
////              HAL_Delay(PUMP_PRESTART_MSG_MS);

////              g_i2c_pause = 1U;
////              g_pump_running = 1U;
////              g_pump_tick = HAL_GetTick();
////              g_state.pump_on = 1U;
////          }
////          else
////          {
////              /* 确保在没有请求时，状态位是准确的 0 */
////              g_state.pump_on = 0U;
////          }
////      }
////      else
////      {
////          /* 正在浇水，强制保持开启状态直到时间结束 */
////          g_state.pump_on = 1U;

////          if ((now_tick - g_pump_tick) >= PUMP_RUN_MS)
////          {
////              g_state.pump_on = 0U;
////              /* 立即更新硬件，确保水泵瞬间停止 */
////              BSP_Output_Update(&g_state);

////              g_pump_running = 0U;
////              HAL_Delay(PUMP_RECOVER_DELAY_MS);

////              OLED_RecoverAfterPump();
////              g_i2c_pause = 0U;
////              g_pump_lock_until = HAL_GetTick() + PUMP_COOLDOWN_MS;
////              g_display_tick = HAL_GetTick();
////          }
////      }

////      /* ========================================================
////       * 任务4：统一更新物理输出（驱动 PB0, PB1, PB13）
////       * ======================================================== */
////      /* 此时 g_state 里的 light_on 和 pump_on 已经是经过覆盖且未被清零的最终值 */
////      BSP_Output_Update(&g_state);

////      /* ========================================================
////       * 任务5：刷新 OLED 显示
////       * ======================================================== */
////      if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
////      {
////          g_display_tick = now_tick;
////          if (g_i2c_pause == 0U)
////          {
////              APP_Display_Show(PAGE_SENSOR, &g_sensor, &g_param, &g_state);
////          }
////      }

////      /* ========================================================
////       * 任务6：OneNET 连接维护
////       * ======================================================== */
////      if (g_state.cloud_ok == 0U)
////      {
////          if ((now_tick - g_onenet_retry_tick) >= ONENET_RETRY_PERIOD_MS)
////          {
////              g_onenet_retry_tick = now_tick;
////              BSP_OLED_ShowString(0, 6, "NET CONNECTING..");
////              BSP_OLED_Refresh();

////              if (BSP_ESP8266_StartOneNET() == HAL_OK)
////              {
////                  g_state.wifi_ok = 1U;
////                  g_state.cloud_ok = 1U;
////                  BSP_OLED_ShowString(0, 6, "CLOUD ONLINE    ");
////              }
////              else
////              {
////                  g_state.wifi_ok = 0U;
////                  g_state.cloud_ok = 0U;
////                  BSP_OLED_ShowString(0, 6, "CLOUD FAILED    ");
////                  g_onenet_retry_tick = now_tick + 10000U;
////              }
////              BSP_OLED_Refresh();
////              g_display_tick = HAL_GetTick();
////              g_sensor_tick = HAL_GetTick();
////          }
////      }
////      else
////      {
////          /* 定时上报逻辑... */
////          if ((now_tick - g_cloud_tick) >= CLOUD_UPLOAD_PERIOD_MS)
////          {
////              g_cloud_tick = now_tick;
////              // APP_Cloud_Upload 逻辑...
////          }
////      }
////      HAL_Delay(10);
////  }
//  
//  while (1)
//  {
//      uint32_t now_tick = HAL_GetTick();
//      ESP_ServiceCmd_t service_cmd;
//      memset(&service_cmd, 0, sizeof(service_cmd));

//      /* 任务0：接收云端指令 */
//      if (BSP_ESP8266_PollService(&service_cmd) != 0U)
//      {
//          APP_HandleServiceCommand(&service_cmd, now_tick);
//          (void)BSP_ESP8266_ReplyService(&service_cmd, 1U);
//      }

//      /* 任务1：传感器更新 */
//      if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
//      {
//          g_sensor_tick = now_tick;
//          if (g_i2c_pause == 0U) { APP_Sensor_Update(&g_sensor); }
//      }

//      /* ========================================================
//       * 任务2：控制逻辑判断 (核心修复：使用临时变量锁定请求)
//       * ======================================================== */
//      
//      // A. 先运行自动逻辑，允许它读写 g_state
//      APP_Control_Run(&g_sensor, &g_param, &g_state);

//      // B. 运行远程覆盖，如果 10s 内有指令，它会覆盖 g_state.pump_on
//      APP_ApplyRemoteOverrides(now_tick, &g_state);

//      // C. 【关键修复】：用一个局部变量把这一轮最终的“开泵意愿”锁死
//      uint8_t final_pump_intent = g_state.pump_on;

//      /* ========================================================
//       * 任务3：水泵执行状态机 (解决冷却和干涉)
//       * ======================================================== */
//      if (g_pump_running == 0U)
//      {
//          // 只有在这里才真正判断是否满足开泵条件
//          if ((final_pump_intent != 0U) && (now_tick >= g_pump_lock_until))
//          {
//              OLED_ShowWatering();
//              HAL_Delay(PUMP_PRESTART_MSG_MS);

//              g_i2c_pause = 1U;
//              g_pump_running = 1U;
//              g_pump_tick = now_tick;
//              g_state.pump_on = 1U; // 真正开启状态
//          }
//          else
//          {
//              g_state.pump_on = 0U; // 确保不满足条件时引脚是灭的
//          }
//      }
//      else
//      {
//          g_state.pump_on = 1U; // 强制锁定开启状态
//          if ((now_tick - g_pump_tick) >= PUMP_RUN_MS)
//          {
//              g_state.pump_on = 0U;
//              BSP_Output_Update(&g_state); // 立即物理关闭
//              g_pump_running = 0U;
//              HAL_Delay(PUMP_RECOVER_DELAY_MS);
//              OLED_RecoverAfterPump();
//              g_i2c_pause = 0U;
//              g_pump_lock_until = HAL_GetTick() + PUMP_COOLDOWN_MS;
//          }
//      }

//      /* ========================================================
//       * 任务4：物理输出 (这是唯一一处真正操作 GPIO 的地方)
//       * ======================================================== */
//      BSP_Output_Update(&g_state);

//      /* 任务5：显示 */
//      if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
//      {
//          g_display_tick = now_tick;
//          if (g_i2c_pause == 0U) { APP_Display_Show(PAGE_SENSOR, &g_sensor, &g_param, &g_state); }
//      }

//      /* 任务6：网络连接维护 (代码略) */
//      // ... 保持你之前的重连逻辑即可 ...
//	 //* 任务6：OneNET 连接维护 + 定时上报属性
////       * ======================================================== */
//      if (g_state.cloud_ok == 0U)
//      {
//          if ((now_tick - g_onenet_retry_tick) >= ONENET_RETRY_PERIOD_MS)
//          {
//              g_onenet_retry_tick = now_tick;

//              /* 【修改 1】：连接前，在屏幕第四行显示提示，化卡顿为合理交互 */
//              BSP_OLED_ShowString(0, 6, "NET CONNECTING..");
//              BSP_OLED_Refresh();

//              if (BSP_ESP8266_StartOneNET() == HAL_OK)
//              {
//                  g_state.wifi_ok  = 1U;
//                  g_state.cloud_ok = 1U;
//                  /* 【修改 2】：连接成功，提示在线 */
//                  BSP_OLED_ShowString(0, 6, "CLOUD ONLINE    "); 
//              }
//              else
//              {
//                  g_state.wifi_ok  = 0U;
//                  g_state.cloud_ok = 0U;
//                  /* 【修改 3】：连接失败，提示离线，并延长重试时间到20秒，留出时间演示本地功能 */
//                  BSP_OLED_ShowString(0, 6, "CLOUD FAILED    "); 
//                  g_onenet_retry_tick = now_tick + 10000U; // 额外增加10秒惩罚等待，避免频繁卡顿
//              }
//              BSP_OLED_Refresh();
//              
//              /* 防止连网太久导致后续的传感器刷新瞬间乱套，重置一下时间戳 */
//              g_display_tick = HAL_GetTick();
//              g_sensor_tick = HAL_GetTick();
//          }
//      }
//      else
//      {
//          /* 已连接时，定时上报属性... (保留您原来的代码不变) */
//          if ((now_tick - g_cloud_tick) >= CLOUD_UPLOAD_PERIOD_MS)
//          // ... 后面不动 ...
//          {
//              g_cloud_tick = now_tick;

////              APP_Cloud_Upload(&g_sensor, &g_state);

////              if (BSP_ESP8266_PublishPropertyJson(APP_Cloud_GetLastPacket()) == HAL_OK)
////              {
////                  g_state.cloud_ok = 1U;
////              }
////              else
////              {
////                  g_state.cloud_ok = 0U;
////              }
//          }
//      }
//      HAL_Delay(10);
//  }

//}

//    /* USER CODE END WHILE */

//    /* USER CODE BEGIN 3 */
//  /* USER CODE END 3 */


///**
//  * @brief System Clock Configuration
//  * @retval None
//  */
//void SystemClock_Config(void)
//{
//  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
//  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

//  /** Initializes the RCC Oscillators according to the specified parameters
//  * in the RCC_OscInitTypeDef structure.
//  */
//  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
//  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
//  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
//  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
//  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
//  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
//  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//  {
//    Error_Handler();
//  }

//  /** Initializes the CPU, AHB and APB buses clocks
//  */
//  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
//  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
//  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

//  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
//  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
//  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
//  {
//    Error_Handler();
//  }
//}

///* USER CODE BEGIN 4 */

///* USER CODE END 4 */

///**
//  * @brief  This function is executed in case of error occurrence.
//  * @retval None
//  */
//void Error_Handler(void)
//{
//  /* USER CODE BEGIN Error_Handler_Debug */
////  __disable_irq();
////  while (1)
////  {
////  }
//  /* USER CODE END Error_Handler_Debug */
//}
//#ifdef USE_FULL_ASSERT
///**
//  * @brief  Reports the name of the source file and the source line number
//  *         where the assert_param error has occurred.
//  * @param  file: pointer to the source file name
//  * @param  line: assert_param error line source number
//  * @retval None
//  */
//void assert_failed(uint8_t *file, uint32_t line)
//{
//  /* USER CODE BEGIN 6 */
////  /* 这里可根据需要加入调试打印 */
//  /* USER CODE END 6 */
//}
//#endif /* USE_FULL_ASSERT */



/* USER CODE BEGIN Header */
///**
//  ******************************************************************************
//  * @file           : main.c
//  * @brief          : Main program body
//  ******************************************************************************
//  * @attention
//  *
//  * Copyright (c) 2026 STMicroelectronics.
//  * All rights reserved.
//  *
//  * This software is licensed under terms that can be found in the LICENSE file
//  * in the root directory of this software component.
//  * If no LICENSE file comes with this software, it is provided AS-IS.
//  *
//  ******************************************************************************
//  */
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
#include "bsp_oled.h"
#include "bsp_output.h"
#include "app_ph.h"
#include "bsp_bh1750.h"
#include "bsp_dht11.h"
#include "bsp_soil.h"
#include "bsp_esp8266.h"
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
    uint8_t active;     /* 是否处于远程强制控制期 */
    uint8_t value;      /* 强制值 0/1 */
    uint32_t until;     /* 到期时间戳 */
} RemoteOverride_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* 水泵单次运行时间
 * 说明：检测到土壤过干后，先运行固定时间，再停泵观察
 */
#define PUMP_RUN_MS             (2000U)

/* 水泵启动前提示保留时间 */
#define PUMP_PRESTART_MSG_MS    (800U)

/* 水泵停止后恢复等待时间 */
#define PUMP_RECOVER_DELAY_MS   (500U)

/* 两次浇水动作之间的最短等待时间
 * 说明：避免水泵频繁启停
 */
#define PUMP_COOLDOWN_MS        (15000U)

/* 当前整机参数
 * 说明：
 * 1. 土壤湿度当前仍用 ADC 原始值，数值越大越干
 * 2. 光照阈值按你室内实测先设为 100 lux
 * 3. pH 报警范围当前先设得宽一点，避免误报
 */
#define SOIL_DRY_TH_RAW         (4200.0f)
#define LIGHT_ON_TH_LUX         (100.0f)
#define PH_ALARM_LOW_TH         (5.0f)
#define PH_ALARM_HIGH_TH        (7.5f)

#define ONENET_RETRY_PERIOD_MS   (10000U)   /* OneNET 重连周期 10 秒 */

#define CLOUD_UPLOAD_PERIOD_MS    (30000U)   /* CLOUD_UPLOAD_PERIOD_MS（上云周期），当前设为5秒 */
#define REMOTE_OVERRIDE_MS      (10000U)   /* 云端服务控制保持 10 秒 */
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

uint8_t  g_i2c_pause = 0U;          /* g_i2c_pause（I2C暂停标志） */
uint8_t  g_pump_running = 0U;       /* g_pump_running（水泵实际运行标志） */
uint32_t g_pump_tick = 0U;          /* g_pump_tick（水泵启动时间戳） */
uint32_t g_pump_lock_until = 0U;    /* g_pump_lock_until（下次允许开泵的最早时刻） */
uint32_t g_cloud_tick = 0U;   /* g_cloud_tick（云端上传调度时间戳） */
RemoteOverride_t g_remote_pump;
RemoteOverride_t g_remote_light;
RemoteOverride_t g_remote_beep;

uint32_t g_onenet_retry_tick = 0U;   /* OneNET 重连调度时间戳 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

static void APP_DefaultParam_Init(SystemParam_t *param);
static void OLED_ShowReady(void);
static void OLED_ShowWatering(void);
static void OLED_I2C_BusRecover(void);
static void OLED_RecoverAfterPump(void);

static void APP_ClearRemoteOverrides(void);
static void APP_ApplyRemoteOverrides(uint32_t now_tick, SystemState_t *state);
static void APP_HandleServiceCommand(const ESP_ServiceCmd_t *cmd, uint32_t now_tick);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* OLED_ShowReady（显示系统就绪界面） */
static void OLED_ShowReady(void)
{
    BSP_OLED_Clear();
    BSP_OLED_ShowString(0, 0, "PLANT SYS");
    BSP_OLED_ShowString(0, 2, "READY");
    BSP_OLED_Refresh();
}

/* OLED_ShowWatering（显示浇水提示界面） */
static void OLED_ShowWatering(void)
{
    BSP_OLED_Clear();
    BSP_OLED_ShowString(0, 0, "PLANT SYS");
    BSP_OLED_ShowString(0, 2, "WATERING...");
    BSP_OLED_Refresh();
}

/* OLED_I2C_BusRecover（I2C总线恢复） */
static void OLED_I2C_BusRecover(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint8_t i = 0U;

    HAL_I2C_DeInit(&hi2c1);

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* 释放总线 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1);

    /* SCL 输出 9 个时钟脉冲 */
    for (i = 0U; i < 9U; i++)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(1);
    }

    /* 产生一次 STOP 条件 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1);

    MX_I2C1_Init();
}

/* OLED_RecoverAfterPump（停泵后恢复 OLED 显示） */
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

/* APP_DefaultParam_Init（默认参数初始化） */
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

/* APP_ClearRemoteOverrides（清空远程强制控制） */
static void APP_ClearRemoteOverrides(void)
{
    memset(&g_remote_pump,  0, sizeof(g_remote_pump));
    memset(&g_remote_light, 0, sizeof(g_remote_light));
    memset(&g_remote_beep,  0, sizeof(g_remote_beep));
}

/* APP_HandleServiceCommand（处理 OneNET 服务调用）
 * 说明：
 * 1. 收到服务调用后，不直接永久改自动控制逻辑
 * 2. 只做一个 10 秒的远程强制控制窗口
 * 3. 到期后系统重新回到自动控制
 */
//static void APP_HandleServiceCommand(const ESP_ServiceCmd_t *cmd, uint32_t now_tick)
//{
//    if (cmd == 0)
//    {
//        return;
//    }

//    switch (cmd->type)
//    {
//        case ESP_SERVICE_SET_PUMP:
//            g_remote_pump.active = 1U;
//            g_remote_pump.value  = cmd->value;
//            g_remote_pump.until  = now_tick + REMOTE_OVERRIDE_MS;
//            break;

//        case ESP_SERVICE_SET_LIGHT:
//            g_remote_light.active = 1U;
//            g_remote_light.value  = cmd->value;
//            g_remote_light.until  = now_tick + REMOTE_OVERRIDE_MS;
//            if (cmd->value == 1U)
//            {
//                BSP_OLED_ShowString(0, 4, "LIGHT CMD: ON ");
//            }
//            else
//            {
//                BSP_OLED_ShowString(0, 4, "LIGHT CMD: OFF");
//            }
//            BSP_OLED_Refresh();
//            break;

//        case ESP_SERVICE_SET_BEEP:
//            g_remote_beep.active = 1U;
//            g_remote_beep.value  = cmd->value;
//            g_remote_beep.until  = now_tick + REMOTE_OVERRIDE_MS;
//            break;

//        default:
//            break;
//    }
//}
static void APP_HandleServiceCommand(const ESP_ServiceCmd_t *cmd, uint32_t now_tick)
{
    if (cmd == 0)
    {
        return;
    }

    switch (cmd->type)
    {
        case ESP_SERVICE_SET_PUMP:
            g_remote_pump.active = 1U;
            g_remote_pump.value  = cmd->value;
            g_remote_pump.until  = now_tick + REMOTE_OVERRIDE_MS+ 5000U;

            if (cmd->value == 1U)
            {
                BSP_OLED_ShowString(0, 4, "PUMP CMD: ON  ");
            }
            else
            {
                BSP_OLED_ShowString(0, 4, "PUMP CMD: OFF ");
            }
            BSP_OLED_Refresh();
            break;

        case ESP_SERVICE_SET_LIGHT:
            g_remote_light.active = 1U;
            g_remote_light.value  = cmd->value;
            g_remote_light.until  = now_tick + REMOTE_OVERRIDE_MS;

            if (cmd->value == 1U)
            {
                BSP_OLED_ShowString(0, 4, "LIGHT CMD: ON ");
            }
            else
            {
                BSP_OLED_ShowString(0, 4, "LIGHT CMD: OFF");
            }
            BSP_OLED_Refresh();
            break;

        case ESP_SERVICE_SET_BEEP:
            g_remote_beep.active = 1U;
            g_remote_beep.value  = cmd->value;
            g_remote_beep.until  = now_tick + REMOTE_OVERRIDE_MS;

            if (cmd->value == 1U)
            {
                BSP_OLED_ShowString(0, 4, "BEEP CMD: ON  ");
            }
            else
            {
                BSP_OLED_ShowString(0, 4, "BEEP CMD: OFF ");
            }
            BSP_OLED_Refresh();
            break;

        default:
            break;
    }
}


/* APP_ApplyRemoteOverrides（应用远程强制控制） */
static void APP_ApplyRemoteOverrides(uint32_t now_tick, SystemState_t *state)
{
    if (state == 0)
    {
        return;
    }

    if ((g_remote_pump.active != 0U) && (now_tick < g_remote_pump.until))
    {
        state->pump_on = g_remote_pump.value;
    }
    else
    {
        g_remote_pump.active = 0U;
    }

    if ((g_remote_light.active != 0U) && (now_tick < g_remote_light.until))
    {
        state->light_on = g_remote_light.value;
    }
    else
    {
        g_remote_light.active = 0U;
    }

    if ((g_remote_beep.active != 0U) && (now_tick < g_remote_beep.until))
    {
        state->beep_on = g_remote_beep.value;
    }
    else
    {
        g_remote_beep.active = 0U;
    }
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

 /* =========================
   * 底层模块初始化
   * =========================
   */

  BSP_OLED_Init();
  BSP_BH1750_Init();
  BSP_DHT11_Init();
  BSP_Soil_Init();
  /* 注意：USART1 已经给 ESP8266 用，不能再初始化调试串口输出 */
  /* =========================
   * 上层参数与状态初始化
   * =========================
   */
  APP_DefaultParam_Init(&g_param);
  APP_Control_Init(&g_state);
  APP_Menu_Init();
  APP_Display_Init();
  APP_Sensor_Init(&g_sensor);
  BSP_Output_Init();
  APP_PH_Init();
  
	  /* 云端与 ESP8266 初始化 */
	APP_Cloud_Init(&g_state);
	BSP_ESP8266_Init();
	APP_ClearRemoteOverrides();

	g_cloud_tick = HAL_GetTick();
	g_onenet_retry_tick = HAL_GetTick();

	g_state.wifi_ok  = 0U;
	g_state.cloud_ok = 0U;
  
  g_sensor_tick  = HAL_GetTick();
  g_display_tick = HAL_GetTick();

	OLED_ShowReady();
	HAL_Delay(300);
  /* 关闭 PC13 板载 LED */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /* =========================
   * 当前整机工作参数
   * =========================
   */
  g_param.soil_low_th   = SOIL_DRY_TH_RAW;     /* 土壤干燥阈值（ADC原始值） */
  g_param.light_low_th  = LIGHT_ON_TH_LUX;     /* 自动补光开启阈值 */
  g_param.ph_low_th     = PH_ALARM_LOW_TH;     /* pH 报警下限 */
  g_param.ph_high_th    = PH_ALARM_HIGH_TH;    /* pH 报警上限 */
  g_param.auto_water_en = 1U;
  g_param.auto_light_en = 1U;
  g_param.alarm_en      = 1U;

  /* 初始状态 */
  g_state.pump_on  = 0U;
  g_state.light_on = 0U;
  g_state.beep_on  = 0U;

  g_i2c_pause      = 0U;
  g_pump_running   = 0U;
  g_pump_tick      = 0U;
  g_pump_lock_until = HAL_GetTick();

//  OLED_ShowReady();
//  HAL_Delay(1000);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

// while (1)
//  {
//      uint32_t now_tick = HAL_GetTick();
//      uint8_t pump_request = 0U;   /* pump_request（自动控制层给出的开泵请求） */
//		
//	  ESP_ServiceCmd_t service_cmd;
//      memset(&service_cmd, 0, sizeof(service_cmd));

//      /* 任务0：轮询 OneNET 服务调用
//       * 说明：
//       * 1. 收到 SetPump / SetLight / SetBeep 的 invoke 消息
//       * 2. 先登记成 10 秒远程强制控制
//       * 3. 再回复 invoke_reply
//       */
//      if (BSP_ESP8266_PollService(&service_cmd) != 0U)
//      {
//          APP_HandleServiceCommand(&service_cmd, now_tick);

//          if (BSP_ESP8266_ReplyService(&service_cmd, 1U) != HAL_OK)
//          {
//              g_state.cloud_ok = 0U;
//          }
//      }
//      /* =========================
//       * 任务1：更新传感器数据
//       * 说明：
//       * 水泵运行期间暂停 I2C 相关访问，减少干扰
//       * =========================
//       */
//      if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
//      {
//          g_sensor_tick = now_tick;

//          if (g_i2c_pause == 0U)
//          {
//              APP_Sensor_Update(&g_sensor);
//          }
//      }

//      /* =========================
//       * 任务2：执行自动控制逻辑
//       * 说明：
//       * 这里先得到“期望状态”
//       * =========================
//       */
//      APP_Control_Run(&g_sensor, &g_param, &g_state);
//		
//	  /* 云端服务控制优先级高于自动控制，但只保持一小段时间 */
//      APP_ApplyRemoteOverrides(now_tick, &g_state);
//	  
//      /* 先保存自动控制层给出的开泵请求 */
//      pump_request = g_state.pump_on;

//      /* =========================
//       * 任务3：水泵实际执行逻辑
//       * 说明：
//       * 1. 检测到土壤过干时，不直接无限开泵
//       * 2. 采用“单次运行 + 冷却等待”的方式
//       * 3. 开泵时暂停 I2C，停泵后恢复 OLED / BH1750
//       * =========================
//       */
//      if (g_pump_running == 0U)
//      {
//          /* 当前未开泵 */
//          g_state.pump_on = 0U;

//          if ((pump_request != 0U) && (now_tick >= g_pump_lock_until))
//          {
//              /* 开泵前先提示 */
//              OLED_ShowWatering();
//              HAL_Delay(PUMP_PRESTART_MSG_MS);

//              /* 开泵并暂停 I2C */
//              g_i2c_pause = 1U;
//              g_pump_running = 1U;
//              g_pump_tick = HAL_GetTick();
//              g_state.pump_on = 1U;
//          }
//      }
//      else
//      {
//          /* 当前正在开泵 */
//          g_state.pump_on = 1U;

//          if ((now_tick - g_pump_tick) >= PUMP_RUN_MS)
//          {
//              /* 到时先关泵 */
//              g_state.pump_on = 0U;
//              BSP_Output_Update(&g_state);

//              g_pump_running = 0U;

//              /* 等待干扰衰减 */
//              HAL_Delay(PUMP_RECOVER_DELAY_MS);

//              /* 恢复 OLED / I2C */
//              OLED_RecoverAfterPump();
//              g_i2c_pause = 0U;

//              /* 设置下次允许开泵的最早时刻 */
//              g_pump_lock_until = HAL_GetTick() + PUMP_COOLDOWN_MS;

//              /* 防止恢复后立刻被刷新覆盖 */
//              g_display_tick = HAL_GetTick();
//          }
//      }

//      /* =========================
//       * 任务4：更新实际输出
//       * 说明：
//       * 1. PB0：水泵
//       * 2. PB1：补光灯
//       * 3. PB13：蜂鸣器（pH异常报警）
//       * =========================
//       */
//      BSP_Output_Update(&g_state);

//      /* =========================
//       * 任务5：刷新 OLED 显示
//       * 说明：
//       * 固定显示传感器页，确保能看到土壤湿度 / 光照 / pH
//       * =========================
//       */
//      if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
//      {
//          g_display_tick = now_tick;

//          if (g_i2c_pause == 0U)
//          {
//              APP_Display_Show(PAGE_SENSOR, &g_sensor, &g_param, &g_state);
//          }
//      }
//		
//	   /* ========================================================
//       * 任务6：OneNET 连接维护 + 定时上报属性
//       * ======================================================== */
//      if (g_state.cloud_ok == 0U)
//      {
//          if ((now_tick - g_onenet_retry_tick) >= ONENET_RETRY_PERIOD_MS)
//          {
//              g_onenet_retry_tick = now_tick;

//              /* 【修改 1】：连接前，在屏幕第四行显示提示，化卡顿为合理交互 */
//              BSP_OLED_ShowString(0, 6, "NET CONNECTING..");
//              BSP_OLED_Refresh();

//              if (BSP_ESP8266_StartOneNET() == HAL_OK)
//              {
//                  g_state.wifi_ok  = 1U;
//                  g_state.cloud_ok = 1U;
//                  /* 【修改 2】：连接成功，提示在线 */
//                  BSP_OLED_ShowString(0, 6, "CLOUD ONLINE    "); 
//              }
//              else
//              {
//                  g_state.wifi_ok  = 0U;
//                  g_state.cloud_ok = 0U;
//                  /* 【修改 3】：连接失败，提示离线，并延长重试时间到20秒，留出时间演示本地功能 */
//                  BSP_OLED_ShowString(0, 6, "CLOUD FAILED    "); 
//                  g_onenet_retry_tick = now_tick + 10000U; // 额外增加10秒惩罚等待，避免频繁卡顿
//              }
//              BSP_OLED_Refresh();
//              
//              /* 防止连网太久导致后续的传感器刷新瞬间乱套，重置一下时间戳 */
//              g_display_tick = HAL_GetTick();
//              g_sensor_tick = HAL_GetTick();
//          }
//      }
//      else
//      {
//          /* 已连接时，定时上报属性... (保留您原来的代码不变) */
//          if ((now_tick - g_cloud_tick) >= CLOUD_UPLOAD_PERIOD_MS)
//          // ... 后面不动 ...
//          {
//              g_cloud_tick = now_tick;

////              APP_Cloud_Upload(&g_sensor, &g_state);

////              if (BSP_ESP8266_PublishPropertyJson(APP_Cloud_GetLastPacket()) == HAL_OK)
////              {
////                  g_state.cloud_ok = 1U;
////              }
////              else
////              {
////                  g_state.cloud_ok = 0U;
////              }
//          }
//      }
//      HAL_Delay(10);
//  }


///* Infinite loop */
//  /* USER CODE BEGIN WHILE */
//  while (1)
//  {
////	  /* 临时调试代码：强制拉高所有控制引脚 */
////HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // 水泵
////HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET); // 补光灯
////HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // 蜂鸣器(低电平触发)
//      uint32_t now_tick = HAL_GetTick();
//      uint8_t pump_request = 0U;
//      ESP_ServiceCmd_t service_cmd;
//      memset(&service_cmd, 0, sizeof(service_cmd));

//      /* ========================================================
//       * 任务0：轮询 OneNET 服务调用
//       * ======================================================== */
//      if (BSP_ESP8266_PollService(&service_cmd) != 0U)
//      {
//          /* 1. 收到云端指令（SetLight / SetPump 等） */
//          APP_HandleServiceCommand(&service_cmd, now_tick);

//          /* 2. 立即回复云端，防止云端显示“响应超时” */
//          if (BSP_ESP8266_ReplyService(&service_cmd, 1U) != HAL_OK)
//          {
//              g_state.cloud_ok = 0U;
//          }
//      }

//      /* ========================================================
//       * 任务1：更新传感器数据
//       * ======================================================== */
//      if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
//      {
//          g_sensor_tick = now_tick;
//          if (g_i2c_pause == 0U)
//          {
//              APP_Sensor_Update(&g_sensor);
//          }
//      }

//      /* ========================================================
//       * 任务2：执行逻辑判断（重点修改区）
//       * ======================================================== */
//      
//      /* A. 先运行本地自动控制逻辑（基于传感器数据判断是否该开灯/开泵） */
//      APP_Control_Run(&g_sensor, &g_param, &g_state);

//      /* B. 【关键】应用远程强制覆盖。如果10秒内有云端指令，它会覆盖上面的本地判断结果 */
//      APP_ApplyRemoteOverrides(now_tick, &g_state);

//      /* C. 将最终确定的期望状态存入请求变量 */
//      pump_request = g_state.pump_on;
//      // 补光灯和蜂鸣器不需要额外状态机，直接使用 g_state.light_on 和 g_state.beep_on 即可

//      /* ========================================================
//       * 任务3：水泵实际执行状态机（单次运行 + 冷却模式）
//       * ======================================================== */
//      if (g_pump_running == 0U)
//      {
//          /* 只有在满足：有开启请求 且 已经过了冷却时间 才会启动 */
//          if ((pump_request != 0U) && (now_tick >= g_pump_lock_until))
//          {
//              OLED_ShowWatering();
//              HAL_Delay(PUMP_PRESTART_MSG_MS);

//              g_i2c_pause = 1U;
//              g_pump_running = 1U;
//              g_pump_tick = HAL_GetTick();
//              g_state.pump_on = 1U;
//          }
//          else
//          {
//              /* 确保在没有请求时，状态位是准确的 0 */
//              g_state.pump_on = 0U;
//          }
//      }
//      else
//      {
//          /* 正在浇水，强制保持开启状态直到时间结束 */
//          g_state.pump_on = 1U;

//          if ((now_tick - g_pump_tick) >= PUMP_RUN_MS)
//          {
//              g_state.pump_on = 0U;
//              /* 立即更新硬件，确保水泵瞬间停止 */
//              BSP_Output_Update(&g_state);

//              g_pump_running = 0U;
//              HAL_Delay(PUMP_RECOVER_DELAY_MS);

//              OLED_RecoverAfterPump();
//              g_i2c_pause = 0U;
//              g_pump_lock_until = HAL_GetTick() + PUMP_COOLDOWN_MS;
//              g_display_tick = HAL_GetTick();
//          }
//      }

//      /* ========================================================
//       * 任务4：统一更新物理输出（驱动 PB0, PB1, PB13）
//       * ======================================================== */
//      /* 此时 g_state 里的 light_on 和 pump_on 已经是经过覆盖且未被清零的最终值 */
//      BSP_Output_Update(&g_state);

//      /* ========================================================
//       * 任务5：刷新 OLED 显示
//       * ======================================================== */
//      if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
//      {
//          g_display_tick = now_tick;
//          if (g_i2c_pause == 0U)
//          {
//              APP_Display_Show(PAGE_SENSOR, &g_sensor, &g_param, &g_state);
//          }
//      }

//      /* ========================================================
//       * 任务6：OneNET 连接维护
//       * ======================================================== */
//      if (g_state.cloud_ok == 0U)
//      {
//          if ((now_tick - g_onenet_retry_tick) >= ONENET_RETRY_PERIOD_MS)
//          {
//              g_onenet_retry_tick = now_tick;
//              BSP_OLED_ShowString(0, 6, "NET CONNECTING..");
//              BSP_OLED_Refresh();

//              if (BSP_ESP8266_StartOneNET() == HAL_OK)
//              {
//                  g_state.wifi_ok = 1U;
//                  g_state.cloud_ok = 1U;
//                  BSP_OLED_ShowString(0, 6, "CLOUD ONLINE    ");
//              }
//              else
//              {
//                  g_state.wifi_ok = 0U;
//                  g_state.cloud_ok = 0U;
//                  BSP_OLED_ShowString(0, 6, "CLOUD FAILED    ");
//                  g_onenet_retry_tick = now_tick + 10000U;
//              }
//              BSP_OLED_Refresh();
//              g_display_tick = HAL_GetTick();
//              g_sensor_tick = HAL_GetTick();
//          }
//      }
//      else
//      {
//          /* 定时上报逻辑... */
//          if ((now_tick - g_cloud_tick) >= CLOUD_UPLOAD_PERIOD_MS)
//          {
//              g_cloud_tick = now_tick;
//              // APP_Cloud_Upload 逻辑...
//          }
//      }
//      HAL_Delay(10);
//  }
  
  while (1)
  {
	static uint8_t g_pub_fail_cnt = 0U;

      uint32_t now_tick = HAL_GetTick();
      ESP_ServiceCmd_t service_cmd;
      memset(&service_cmd, 0, sizeof(service_cmd));

      /* 任务0：接收云端指令 */
      if (BSP_ESP8266_PollService(&service_cmd) != 0U)
      {
          APP_HandleServiceCommand(&service_cmd, now_tick);
          (void)BSP_ESP8266_ReplyService(&service_cmd, 1U);
      }
	
		/* ... 定时上报属性 ... */

//memset(&service_cmd, 0, sizeof(service_cmd));
//if (BSP_ESP8266_PollService(&service_cmd) != 0U)
//{
//    APP_HandleServiceCommand(&service_cmd, now_tick);
//    (void)BSP_ESP8266_ReplyService(&service_cmd, 1U);
//}
	  
      /* 任务1：传感器更新 */
      if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
      {
          g_sensor_tick = now_tick;
          if (g_i2c_pause == 0U) { APP_Sensor_Update(&g_sensor); }
      }

      /* ========================================================
       * 任务2：控制逻辑判断 (核心修复：使用临时变量锁定请求)
       * ======================================================== */
      
      // A. 先运行自动逻辑，允许它读写 g_state
      APP_Control_Run(&g_sensor, &g_param, &g_state);

      // B. 运行远程覆盖，如果 10s 内有指令，它会覆盖 g_state.pump_on
      APP_ApplyRemoteOverrides(now_tick, &g_state);

      // C. 【关键修复】：用一个局部变量把这一轮最终的“开泵意愿”锁死
      uint8_t final_pump_intent = g_state.pump_on;

      /* ========================================================
       * 任务3：水泵执行状态机 (解决冷却和干涉)
       * ======================================================== */
      if (g_pump_running == 0U)
      {
          // 只有在这里才真正判断是否满足开泵条件
          if ((final_pump_intent != 0U) && (now_tick >= g_pump_lock_until))
          {
              OLED_ShowWatering();
              HAL_Delay(PUMP_PRESTART_MSG_MS);

              g_i2c_pause = 1U;
              g_pump_running = 1U;
              g_pump_tick = now_tick;
              g_state.pump_on = 1U; // 真正开启状态
          }
          else
          {
              g_state.pump_on = 0U; // 确保不满足条件时引脚是灭的
          }
      }
      else
      {
          g_state.pump_on = 1U; // 强制锁定开启状态
          if ((now_tick - g_pump_tick) >= PUMP_RUN_MS)
          {
              g_state.pump_on = 0U;
              BSP_Output_Update(&g_state); // 立即物理关闭
              g_pump_running = 0U;
              HAL_Delay(PUMP_RECOVER_DELAY_MS);
              OLED_RecoverAfterPump();
              g_i2c_pause = 0U;
              g_pump_lock_until = HAL_GetTick() + PUMP_COOLDOWN_MS;
          }
      }

      /* ========================================================
       * 任务4：物理输出 (这是唯一一处真正操作 GPIO 的地方)
       * ======================================================== */
      BSP_Output_Update(&g_state);

      /* 任务5：显示 */
      if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
      {
          g_display_tick = now_tick;
          if (g_i2c_pause == 0U) { APP_Display_Show(PAGE_SENSOR, &g_sensor, &g_param, &g_state); }
      }

      /* 任务6：网络连接维护 (代码略) */
      // ... 保持你之前的重连逻辑即可 ...
	 //* 任务6：OneNET 连接维护 + 定时上报属性
//       * ======================================================== */
      if (g_state.cloud_ok == 0U)
      {
          if ((now_tick - g_onenet_retry_tick) >= ONENET_RETRY_PERIOD_MS)
          {
              g_onenet_retry_tick = now_tick;

              /* 【修改 1】：连接前，在屏幕第四行显示提示，化卡顿为合理交互 */
              BSP_OLED_ShowString(0, 6, "NET CONNECTING..");
              BSP_OLED_Refresh();

              if (BSP_ESP8266_StartOneNET() == HAL_OK)
              {
                  g_state.wifi_ok  = 1U;
                  g_state.cloud_ok = 1U;
                  /* 【修改 2】：连接成功，提示在线 */
                  BSP_OLED_ShowString(0, 6, "CLOUD ONLINE    "); 
              }
              else
              {
                  g_state.wifi_ok  = 0U;
                  g_state.cloud_ok = 0U;
                  /* 【修改 3】：连接失败，提示离线，并延长重试时间到20秒，留出时间演示本地功能 */
                  BSP_OLED_ShowString(0, 6, "CLOUD FAILED    "); 
                  g_onenet_retry_tick = now_tick + 10000U; // 额外增加10秒惩罚等待，避免频繁卡顿
              }
              BSP_OLED_Refresh();
              
              /* 防止连网太久导致后续的传感器刷新瞬间乱套，重置一下时间戳 */
              g_display_tick = HAL_GetTick();
              g_sensor_tick = HAL_GetTick();
          }
      }
      else
      {
          /* 已连接时，定时上报属性... (保留您原来的代码不变) */
//          if ((now_tick - g_cloud_tick) >= CLOUD_UPLOAD_PERIOD_MS)
//          // ... 后面不动 ...
//          {
//              g_cloud_tick = now_tick;

//              APP_Cloud_Upload(&g_sensor, &g_state);

//              if (BSP_ESP8266_PublishPropertyJson(APP_Cloud_GetLastPacket()) == HAL_OK)
//              {
//                  g_state.cloud_ok = 1U;
//              }
//              else
//              {
//                  g_state.cloud_ok = 0U;
//              }
//          }
		  
		  if ((now_tick - g_cloud_tick) >= CLOUD_UPLOAD_PERIOD_MS)
{
    g_cloud_tick = now_tick;

    APP_Cloud_Upload(&g_sensor, &g_state);

    if (BSP_ESP8266_PublishPropertyJson(APP_Cloud_GetLastPacket()) == HAL_OK)
    {
        g_pub_fail_cnt = 0U;
        g_state.cloud_ok = 1U;
    }
    else
    {
        if (g_pub_fail_cnt < 3U)
        {
            g_pub_fail_cnt++;
        }

        if (g_pub_fail_cnt >= 3U)
        {
            g_state.cloud_ok = 0U;
        }
    }
}
      }
      HAL_Delay(10);
  }


//while (1)
//{
//    /* 开泵：如果你的驱动模块是高电平触发，这里会启动水泵 */
//    HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_SET);

//    BSP_OLED_Clear();
//    BSP_OLED_ShowString(0, 0, "PUMP TEST");
//    BSP_OLED_ShowString(0, 2, "PUMP = ON");
//    BSP_OLED_Refresh();

//    HAL_Delay(3000);

//    /* 关泵 */
//    HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_RESET);

//    BSP_OLED_Clear();
//    BSP_OLED_ShowString(0, 0, "PUMP TEST");
//    BSP_OLED_ShowString(0, 2, "PUMP = OFF");
//    BSP_OLED_Refresh();

//    HAL_Delay(3000);
//}

}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */


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
//  __disable_irq();
//  while (1)
//  {
//  }
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
//  /* 这里可根据需要加入调试打印 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */




