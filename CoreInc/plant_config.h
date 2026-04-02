#ifndef __PLANT_CONFIG_H
#define __PLANT_CONFIG_H

/* USE_MOCK_DATA（是否启用模拟数据）
 * 1：当前使用模拟数据，适合没有硬件时进行软件逻辑开发
 * 0：后续切换为真实硬件采集
 */
#define USE_MOCK_DATA           (1U)

/* 软件调度周期，单位：ms */
#define SENSOR_PERIOD_MS        (500U)   /* 传感器更新周期 */
#define DISPLAY_PERIOD_MS       (500U)   /* 显示刷新周期 */
#define CLOUD_PERIOD_MS         (1000U)  /* 云端上传周期 */

/* 默认参数阈值 */
#define DEF_SOIL_LOW_TH         (30.0f)  /* 默认土壤湿度下限阈值 */
#define DEF_LIGHT_LOW_TH        (400.0f) /* 默认光照下限阈值 */
#define DEF_PH_LOW_TH           (5.5f)   /* 默认pH下限阈值 */
#define DEF_PH_HIGH_TH          (7.5f)   /* 默认pH上限阈值 */

/* =========================================================
 *  pH 模块软件换算参数（按你提供的 3.3V ADC 资料先设默认值）
 * =========================================================
 */

/* STM32F103 12 位 ADC 最大计数值 */
#define PH_ADC_MAX_VALUE        (4095U)

/* STM32 ADC 参考电压 */
#define PH_ADC_VREF             (3.3f)

/* 3.3V ADC 拟合默认公式：
 * pH = PH_DEFAULT_K * voltage + PH_DEFAULT_B
 * 该参数来自你提供的 pH 模块资料中的 3.3V 曲线拟合
 */
#define PH_DEFAULT_K            (-5.7541f)
#define PH_DEFAULT_B            (16.654f)

/* pH 合法范围 */
#define PH_MIN_VALUE            (0.0f)
#define PH_MAX_VALUE            (14.0f)

#endif
