#ifndef __APP_SENSOR_H
#define __APP_SENSOR_H

#include "plant_types.h"

/* APP_Sensor_Init（传感器数据初始化）
 * 作用：初始化传感器数据结构体，避免出现随机值
 */
void APP_Sensor_Init(SensorData_t *data);

/* APP_Sensor_Update（传感器数据更新）
 * 作用：统一更新系统的实时环境数据
 * 当前阶段使用模拟数据，后续可切换为真实硬件采集
 */
void APP_Sensor_Update(SensorData_t *data);

#endif
