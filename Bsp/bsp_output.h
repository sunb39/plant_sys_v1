#ifndef __BSP_OUTPUT_H
#define __BSP_OUTPUT_H

#include "bsp_board.h"
#include "plant_types.h"
#include <stdint.h>

/* BSP_Output_Init（输出模块初始化）
 * 作用：系统上电后将所有执行器默认关闭
 */
void BSP_Output_Init(void);

/* BSP_Pump_Set（水泵控制）
 * on=1 开启水泵，on=0 关闭水泵
 */
void BSP_Pump_Set(uint8_t on);

/* BSP_Light_Set（补光控制）
 * on=1 开启补光，on=0 关闭补光
 */
void BSP_Light_Set(uint8_t on);

/* BSP_Beep_Set（蜂鸣器控制）
 * on=1 开启报警，on=0 关闭报警
 */
void BSP_Beep_Set(uint8_t on);

/* BSP_Output_Update（根据系统状态统一更新输出）
 * 作用：把应用层状态结构体映射到实际 GPIO 输出
 */
void BSP_Output_Update(const SystemState_t *state);

#endif
