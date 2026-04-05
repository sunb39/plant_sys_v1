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
uint8_t g_last_pump_on = 0U;      /* 上一次水泵状态 */
uint8_t g_display_pause = 0U;     /* OLED 刷新暂停标志 */

//uint8_t g_last_pump_on = 0U;         /* g_last_pump_on（记录上一次水泵状态） */
uint8_t g_i2c_pause = 0U;            /* g_i2c_pause（I2C暂停标志，1表示暂停OLED与BH1750） */
uint32_t g_pump_tick = 0U;           /* g_pump_tick（水泵节拍时间戳） */
uint8_t g_pump_phase_on = 0U;        /* g_pump_phase_on（水泵当前阶段，1开/0关） */
/* USER CODE END PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void APP_DefaultParam_Init(SystemParam_t *param);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* OLED_I2C_BusRecover（I2C总线恢复）
 * 作用：
 * 1. 当水泵启动后电磁干扰导致 I2C 总线异常时，尝试通过手动时钟脉冲恢复总线
 * 2. 该函数会先释放 I2C 外设，再将 PB6/PB7 临时改成 GPIO 开漏输出
 * 3. 通过给 SCL（PB6）输出若干个脉冲，尝试释放被挂住的 SDA（PB7）
 * 4. 最后重新初始化 I2C1 外设
 *
 * 说明：
 * 1. PB6 = I2C1_SCL
 * 2. PB7 = I2C1_SDA
 * 3. 这是典型的 I2C 总线恢复手段，适合当前“水泵干扰后 OLED 卡在最后一帧”的情况
 */
static void OLED_I2C_BusRecover(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint8_t i = 0;

    /* 先反初始化 I2C 外设，释放 PB6/PB7 控制权 */
    HAL_I2C_DeInit(&hi2c1);

    /* 将 PB6 / PB7 临时配置为开漏输出 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

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

    /* 产生一次 STOP 条件：SCL高时，SDA从低释放到高 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1);

    /* 重新初始化 I2C1 */
    MX_I2C1_Init();
}
//单词浇水，停泵后总线恢复
/* OLED_RecoverAfterPump（停泵后恢复 OLED 显示）
 * 作用：
 * 1. 停泵后先恢复 I2C 总线
 * 2. 再重新初始化 OLED
 * 3. 最后重新显示“PUMP DONE”
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
////测试oled不刷新，水泵工作是否干扰
//int main(void)
//{
//    /* USER CODE BEGIN 1 */

//    /* USER CODE END 1 */

//    /* MCU Configuration--------------------------------------------------------*/
//    HAL_Init();
//    SystemClock_Config();

//    /* 这里只初始化本次实验真正需要的外设
//     * 1. GPIO：用于控制 PB0（水泵控制脚）
//     * 2. I2C1：仅用于 OLED 上电后显示一次固定字符串
//     */
//    MX_GPIO_Init();
//    MX_I2C1_Init();

//    /* USER CODE BEGIN 2 */
//    /* 先确保水泵关闭，避免系统刚上电时误动作
//     * WATER_PUMP_CTRL（水泵控制脚）= PB0
//     */
//    HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_RESET);

//    /* 关闭板载 PC13 指示灯
//     * 说明：多数 Blue Pill 小板为低电平点亮、高电平熄灭
//     */
//    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

//    /* =========================
//     * OLED 仅初始化并显示一次固定字符串
//     * 说明：
//     * 1. 这里故意不再调用 APP_Display_Show()、APP_Sensor_Update() 等上层逻辑
//     * 2. 进入 while(1) 后，不再访问 OLED / I2C
//     * 3. 如果此时水泵一启动 OLED 仍然卡死，就可基本排除“上层软件逻辑导致”
//     * =========================
//     */
//    BSP_OLED_Init();
//    BSP_OLED_Clear();
//    BSP_OLED_ShowString(0, 0, "OLED STATIC");
//    BSP_OLED_ShowString(0, 2, "PUMP TEST PB0");
//    BSP_OLED_Refresh();

//    /* 先等待 5 秒，确认 OLED 已稳定显示 */
//    HAL_Delay(5000);
//    /* USER CODE END 2 */

//    /* Infinite loop */
//    while (1)
//    {
//        /* =========================================
//         * 水泵打开测试
//         * 说明：
//         * 1. 该 MOSFET 模块为高电平启动
//         * 2. PB0 输出高电平时，MOSFET 导通，水泵启动
//         * 3. 本阶段 while(1) 中不再执行任何 I2C、传感器、云端、控制逻辑
//         * =========================================
//         */
//        HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_SET);
//        HAL_Delay(2000);   /* 水泵运行 2 秒 */

//        /* =========================================
//         * 水泵关闭测试
//         * 说明：
//         * PB0 输出低电平时，MOSFET 关断，水泵停止
//         * =========================================
//         */
//        HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_RESET);
//        HAL_Delay(15000);  /* 水泵停止 15 秒 */
//    }
//}


////测试oled刷新，水泵是否干扰
//int main(void)
//{
//    uint32_t pump_tick = 0U;       /* pump_tick（水泵节拍时间戳） */
//    uint32_t display_tick = 0U;    /* display_tick（OLED刷新时间戳） */
//    uint8_t pump_on = 0U;          /* pump_on（水泵状态标志） */
//    uint32_t counter = 0U;         /* counter（显示计数器） */
//    char line_buf[24];             /* line_buf（显示字符串缓存） */

//    HAL_Init();
//    SystemClock_Config();

//    /* 这里只初始化本次实验真正需要的外设
//     * 1. GPIO：用于控制 PB0（水泵控制脚）
//     * 2. I2C1：用于 OLED 动态刷新
//     */
//    MX_GPIO_Init();
//    MX_I2C1_Init();

//    /* 上电先关闭水泵，避免误动作 */
//    HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_RESET);

//    /* 关闭板载 PC13 指示灯
//     * 说明：多数 Blue Pill 小板为低电平点亮、高电平熄灭
//     */
//    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

//    /* 初始化 OLED */
//    BSP_OLED_Init();
//    BSP_OLED_Clear();
//    BSP_OLED_ShowString(0, 0, "OLED DYNAMIC");
//    BSP_OLED_ShowString(0, 2, "PUMP TEST A");
//    BSP_OLED_Refresh();

//    /* 系统稳定等待 */
//    HAL_Delay(3000);

//    pump_tick = HAL_GetTick();
//    display_tick = HAL_GetTick();

//    while (1)
//    {
//        uint32_t now_tick = HAL_GetTick();

//        /* =========================================
//         * 任务1：水泵节拍控制
//         * 说明：
//         * 1. 采用“开2秒、停15秒”的方式测试
//         * 2. 这里只验证 PB0 + MOSFET + 水泵链路
//         * =========================================
//         */
//        if (pump_on == 0U)
//        {
//            if ((now_tick - pump_tick) >= 15000U)
//            {
//                pump_tick = now_tick;
//                pump_on = 1U;
//                HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_SET);
//            }
//        }
//        else
//        {
//            if ((now_tick - pump_tick) >= 2000U)
//            {
//                pump_tick = now_tick;
//                pump_on = 0U;
//                HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_RESET);
//            }
//        }

//        /* =========================================
//         * 任务2：OLED 动态刷新
//         * 说明：
//         * 1. 每500ms刷新一次
//         * 2. 这里只做 OLED 写操作，不访问任何传感器
//         * 3. 用于判断“动态 OLED 本身”是否在水泵工作时会出问题
//         * =========================================
//         */
//        if ((now_tick - display_tick) >= 500U)
//        {
//            display_tick = now_tick;
//            counter++;

//            BSP_OLED_Clear();
//            BSP_OLED_ShowString(0, 0, "OLED DYNAMIC");
//            BSP_OLED_ShowString(0, 2, pump_on ? "PUMP: ON " : "PUMP: OFF");

//            sprintf(line_buf, "CNT:%lu", counter);
//            BSP_OLED_ShowString(0, 4, line_buf);

//            BSP_OLED_Refresh();
//        }

//        HAL_Delay(10);
//    }
//}


////单次浇水演示版


//int main(void)
//{
//    HAL_Init();
//    SystemClock_Config();

//    /* 只初始化当前演示真正需要的外设
//     * 1. GPIO：用于控制 PB0（水泵控制脚）
//     * 2. I2C1：仅用于 OLED 在泵工作前后各显示一次
//     */
//    MX_GPIO_Init();
//    MX_I2C1_Init();

//    /* 上电先关闭水泵，避免误动作 */
//    HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_RESET);

//    /* 关闭板载 PC13 指示灯
//     * 说明：多数 Blue Pill 小板为低电平点亮、高电平熄灭
//     */
//    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

//    /* =========================
//     * 阶段1：系统准备完成提示
//     * 说明：
//     * 上电后先显示系统已就绪，确认 OLED 正常工作
//     * =========================
//     */
//    BSP_OLED_Init();
//    BSP_OLED_Clear();
//    BSP_OLED_ShowString(0, 0, "PLANT SYS");
//    BSP_OLED_ShowString(0, 2, "READY");
//    BSP_OLED_Refresh();

//    HAL_Delay(5000);

//    /* =========================
//     * 阶段2：显示即将浇水提示
//     * 说明：
//     * 在水泵真正启动前，先在 OLED 上提示当前状态
//     * =========================
//     */
//    BSP_OLED_Clear();
//    BSP_OLED_ShowString(0, 0, "PLANT SYS");
//    BSP_OLED_ShowString(0, 2, "WATERING...");
//    BSP_OLED_Refresh();

//    /* 给用户一点时间看到提示 */
//    HAL_Delay(1000);

//    /* =========================
//     * 阶段3：停止所有 OLED/I2C 操作，启动水泵
//     * 说明：
//     * 1. 从这里开始不再访问 OLED
//     * 2. 避免水泵工作时 I2C 动态刷新被干扰
//     * =========================
//     */
//    HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_SET);
//    HAL_Delay(2000);   /* 水泵运行 2 秒 */

//    /* 停止水泵 */
//    HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_RESET);

//    /* 等待电机干扰衰减 */
//    HAL_Delay(500);

//    /* =========================
//     * 阶段4：重新初始化 OLED，并显示浇水完成
//     * 说明：
//     * 水泵停止后重新初始化 OLED，恢复显示链路
//     * =========================
//     */
//    BSP_OLED_Init();
//    BSP_OLED_Clear();
//    BSP_OLED_ShowString(0, 0, "PLANT SYS");
//    BSP_OLED_ShowString(0, 2, "PUMP DONE");
//    BSP_OLED_Refresh();

//    while (1)
//    {
//        /* 当前单次演示版不再做循环刷新
//         * 作用：
//         * 1. 验证“显示-执行器-恢复显示”的分阶段方案是否稳定
//         * 2. 便于后续在此基础上改成正式版流程
//         */
//        HAL_Delay(1000);
//    }
//}



//单次浇水停泵后总线恢复

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    /* 只初始化当前实验真正需要的外设
     * 1. GPIO：用于控制 PB0（水泵控制）
     * 2. I2C1：用于 OLED 显示
     */
    MX_GPIO_Init();
    MX_I2C1_Init();

    /* 上电先关闭水泵，避免误动作 */
    HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_RESET);

    /* 关闭板载 PC13 灯 */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    /* =========================
     * 阶段1：显示系统已就绪
     * =========================
     */
    BSP_OLED_Init();
    BSP_OLED_Clear();
    BSP_OLED_ShowString(0, 0, "PLANT SYS");
    BSP_OLED_ShowString(0, 2, "READY");
    BSP_OLED_Refresh();

    HAL_Delay(5000);

    /* =========================
     * 阶段2：显示即将浇水
     * =========================
     */
    BSP_OLED_Clear();
    BSP_OLED_ShowString(0, 0, "PLANT SYS");
    BSP_OLED_ShowString(0, 2, "WATERING...");
    BSP_OLED_Refresh();

    HAL_Delay(1000);

    /* =========================
     * 阶段3：停止一切 OLED/I2C 操作，启动水泵
     * 说明：
     * 从这里开始不再访问 OLED，避免水泵工作期间动态刷新
     * =========================
     */
    HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_SET);
    HAL_Delay(2000);   /* 水泵运行 2 秒 */

    /* 停止水泵 */
    HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_RESET);

    /* 等待干扰衰减 */
    HAL_Delay(500);

    /* =========================
     * 阶段4：恢复 I2C + 恢复 OLED
     * =========================
     */
    OLED_RecoverAfterPump();

    while (1)
    {
        HAL_Delay(1000);
    }
}


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

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
//  while (1)
//{
//    uint32_t now_tick = HAL_GetTick();
//    const DisplayCache_t *disp = 0;

//    /* 任务1：传感器数据更新 + 控制逻辑计算 */
//    if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
//    {
//        g_sensor_tick = now_tick;

//        APP_Sensor_Update(&g_sensor);
//        APP_Control_Run(&g_sensor, &g_param, &g_state);
//			/* =========================
//	 * 临时整机联调测试：强制水泵按节奏启停
//	 * 作用：
//	 * 1. 验证整机版下 g_state.pump_on 是否能正确驱动水泵
//	 * 2. 当前属于联调阶段测试代码，后续确认无误后删除
//	 * =========================
//	 */
//	if ((now_tick / 5000U) % 2U == 0U)
//	{
//		g_state.pump_on = 1U;   /* 每 5 秒进入一次开泵状态 */
//	}
//	else
//	{
//		g_state.pump_on = 0U;   /* 每 5 秒进入一次停泵状态 */
//	}
//				BSP_Output_Update(&g_state);
//    }

//    /* 任务2：界面显示刷新 */
//    if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
//    {
//        g_display_tick = now_tick;

//        APP_Display_Show(APP_Menu_GetPage(), &g_sensor, &g_param, &g_state);
//    }

//    /* 任务3：云端数据上传 */
//    if ((now_tick - g_cloud_tick) >= CLOUD_PERIOD_MS)
//    {
//        g_cloud_tick = now_tick;

//        APP_Cloud_Upload(&g_sensor, &g_state);

//        BSP_Debug_Printf("T_x10=%ld,H_x10=%ld,L_x10=%ld,S_x10=%ld,PH_x100=%ld,P=%d,LED=%d,BEEP=%d\r\n",
//                         (long)(g_sensor.air_temp * 10.0f),
//                         (long)(g_sensor.air_humi * 10.0f),
//                         (long)(g_sensor.light_lux * 10.0f),
//                         (long)(g_sensor.soil_moisture * 10.0f),
//                         (long)(g_sensor.ph_value * 100.0f),
//                         g_state.pump_on,
//                         g_state.light_on,
//                         g_state.beep_on);

//        BSP_Debug_Printf("CloudPacket: %s\r\n", APP_Cloud_GetLastPacket());

//        disp = APP_Display_GetCache();
//        BSP_Debug_Printf("PAGE:\r\n%s\r\n%s\r\n%s\r\n%s\r\n",
//                         disp->line1,
//                         disp->line2,
//                         disp->line3,
//                         disp->line4);
//    }

//    /* 任务4：云端报文解析 */
//    APP_Cloud_Parse();

//    HAL_Delay(10);
//    /* USER CODE END WHILE */

//    /* USER CODE BEGIN 3 */

//  }

//while (1)
//{
//    uint32_t now_tick = HAL_GetTick();

//    /* =========================
//     * 任务1：传感器数据更新
//     * =========================
//     */
//    if ((now_tick - g_sensor_tick) >= SENSOR_PERIOD_MS)
//    {
//        g_sensor_tick = now_tick;
//        APP_Sensor_Update(&g_sensor);
//    }

//    /* =========================
//     * 任务2：临时强制水泵启停测试
//     * 说明：
//     * 当前仍处于整机联调阶段，为验证执行器链路，
//     * 先采用每5秒切换一次水泵状态的方式进行测试。
//     * =========================
//     */
//    if ((now_tick / 5000U) % 2U == 0U)
//    {
//        g_state.pump_on = 1U;
//    }
//    else
//    {
//        g_state.pump_on = 0U;
//    }

//    /* 当前阶段先关闭补光和蜂鸣器，减少联调变量 */
//    g_state.light_on = 0U;
//    g_state.beep_on  = 0U;

//    /* =========================
//     * 任务3：水泵状态变化检测
//     * 说明：
//     * 1. 水泵启动时，暂停 OLED 刷新，降低 I2C 被干扰概率
//     * 2. 水泵停止后，重新初始化 OLED，恢复显示
//     * =========================
//     */
//    if (g_state.pump_on != g_last_pump_on)
//    {
//        g_last_pump_on = g_state.pump_on;

//        if (g_state.pump_on)
//        {
//            /* 水泵启动：暂停 OLED 刷新 */
//            g_display_pause = 1U;
//        }
//        else
//        {
//            /* 水泵停止：等待电机干扰消失后重新初始化 OLED */
//            HAL_Delay(100);
//            BSP_OLED_Init();
//            APP_Display_Init();
//            g_display_pause = 0U;
//        }
//    }

//    /* 更新执行器输出 */
//    BSP_Output_Update(&g_state);

//    /* =========================
//     * 任务4：OLED 显示刷新
//     * 说明：
//     * 仅在未暂停显示时刷新 OLED
//     * =========================
//     */
//    if ((now_tick - g_display_tick) >= DISPLAY_PERIOD_MS)
//    {
//        g_display_tick = now_tick;

//        if (g_display_pause == 0U)
//        {
//            APP_Display_Show(APP_Menu_GetPage(), &g_sensor, &g_param, &g_state);
//        }
//    }

//    HAL_Delay(10);
//}

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

