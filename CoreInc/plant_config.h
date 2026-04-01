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

#endif
