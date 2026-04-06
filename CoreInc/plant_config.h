#ifndef __PLANT_CONFIG_H
#define __PLANT_CONFIG_H

/* USE_MOCK_DATA（是否启用模拟数据）
 * 1：当前使用模拟数据，适合没有硬件时进行软件逻辑开发
 * 0：后续切换为真实硬件采集
 */
//#define USE_MOCK_DATA           (1U)
#define USE_MOCK_DHT11   0
#define USE_MOCK_BH1750  0
#define USE_MOCK_SOIL    0
#define USE_MOCK_PH      0
/* 软件调度周期，单位：ms */
#define SENSOR_PERIOD_MS        (500U)   /* 传感器更新周期 */
#define DISPLAY_PERIOD_MS       (1000U)   /* 显示刷新周期 */
#define CLOUD_PERIOD_MS         (1000U)  /* 云端上传周期 */

/* DHT11 单独采样周期，建议 >= 1000ms，联调阶段用 2000ms 更稳 */
#define DHT11_PERIOD_MS         (2000U)

/* 默认参数阈值 */
#define DEF_SOIL_LOW_TH         (30.0f)  /* 默认土壤湿度下限阈值 */
#define DEF_LIGHT_LOW_TH        (400.0f) /* 默认光照下限阈值 */
#define DEF_PH_LOW_TH           (5.5f)   /* 默认pH下限阈值 */
#define DEF_PH_HIGH_TH          (7.5f)   /* 默认pH上限阈值 */

/* =========================================================
 *  pH 模块软件换算参数（按你提供的 3.3V ADC 资料先设默认值）
 * =========================================================
 */

//#define USE_MOCK_PH          (0U)        /* USE_MOCK_PH（pH模拟开关），0=真实硬件 */

#define PH_ADC_VREF          (3.3f)      /* PH_ADC_VREF（ADC参考电压） */
#define PH_ADC_MAX_VALUE     (4095U)     /* PH_ADC_MAX_VALUE（12位ADC最大计数） */

#define PH_MIN_VALUE         (0.0f)      /* PH_MIN_VALUE（pH最小值） */
#define PH_MAX_VALUE         (14.0f)     /* PH_MAX_VALUE（pH最大值） */

/* 下面这组参数，按你手册里的 3.3V_ADC 拟合曲线先给默认值
 * 说明：
 * 1. 这组值适用于“5V pH模块 + 10k/10k 分压后送入 STM32 PA1”的场景
 * 2. 后续你如果用缓冲液重新标定，再改成最终值
 */
#define PH_DEFAULT_K         (-6.3951f)  /* PH_DEFAULT_K（默认斜率） */
#define PH_DEFAULT_B         (14.8716f)   /* PH_DEFAULT_B（默认截距） */

/* 水泵联调测试参数
 * 说明：
 * 1. 当前用于整机联调阶段
 * 2. 水泵工作时间不宜过长，防止连续干扰显示链路
 */
#define PUMP_TEST_ON_MS         (2000U)   /* 水泵开启 2 秒 */
#define PUMP_TEST_OFF_MS        (15000U)  /* 水泵关闭 15 秒 */

/* 水泵停止后，等待干扰衰减再恢复 OLED */
#define PUMP_RECOVER_DELAY_MS   (500U)
#endif
