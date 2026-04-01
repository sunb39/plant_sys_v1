#ifndef __APP_CONTROL_H
#define __APP_CONTROL_H

#include "plant_types.h"

/* APP_Control_Init（系统状态初始化）
 * 作用：初始化执行机构状态和通信状态
 */
void APP_Control_Init(SystemState_t *state);

/* APP_Control_Run（控制逻辑运行）
 * 作用：根据实时传感器数据和参数阈值，输出系统控制状态
 */
void APP_Control_Run(const SensorData_t *data, const SystemParam_t *param, SystemState_t *state);

#endif
