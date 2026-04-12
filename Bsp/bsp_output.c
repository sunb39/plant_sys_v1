//#include "bsp_output.h"

///* BSP_Output_Write（底层输出辅助函数）
// * 作用：
// * 根据“有效电平”自动转换 GPIO 输出状态
// * 这样以后如果模块是低电平触发，只改宏定义即可，不用改函数逻辑
// */
//static void BSP_Output_Write(GPIO_TypeDef *port,
//                             uint16_t pin,
//                             GPIO_PinState active_level,
//                             GPIO_PinState inactive_level,
//                             uint8_t on)
//{
//    if (on)
//    {
//        HAL_GPIO_WritePin(port, pin, active_level);
//    }
//    else
//    {
//        HAL_GPIO_WritePin(port, pin, inactive_level);
//    }
//}

///* BSP_Output_Init（执行器输出初始化）
// * 说明：系统上电默认全部关闭，避免误动作
// */
//void BSP_Output_Init(void)
//{
//    BSP_Pump_Set(0);
//    BSP_Light_Set(0);
//    BSP_Beep_Set(0);
//}

///* BSP_Pump_Set（水泵开关控制）
// * 说明：你的 MOSFET 水泵驱动资料写的是“高电平启动”
// */
//void BSP_Pump_Set(uint8_t on)
//{
//    BSP_Output_Write(PUMP_GPIO_PORT,
//                     PUMP_GPIO_PIN,
//                     PUMP_ACTIVE_LEVEL,
//                     PUMP_INACTIVE_LEVEL,
//                     on);
//}

///* BSP_Light_Set（补光输出控制） */
//void BSP_Light_Set(uint8_t on)
//{
//    BSP_Output_Write(LIGHT_GPIO_PORT,
//                     LIGHT_GPIO_PIN,
//                     LIGHT_ACTIVE_LEVEL,
//                     LIGHT_INACTIVE_LEVEL,
//                     on);
//}

///* BSP_Beep_Set（蜂鸣器控制）
// * 说明：如果后面你实物是低电平触发蜂鸣器，只需要改 bsp_board.h 里的有效电平宏
// */
//void BSP_Beep_Set(uint8_t on)
//{
//    BSP_Output_Write(BEEP_GPIO_PORT,
//                     BEEP_GPIO_PIN,
//                     BEEP_ACTIVE_LEVEL,
//                     BEEP_INACTIVE_LEVEL,
//                     on);
//}

///* BSP_Output_Update（根据系统状态统一更新输出）
// * 说明：
// * 这个函数建议放在主循环中调用，
// * 这样应用层只负责“判断状态”，输出层只负责“执行动作”。
// */
//void BSP_Output_Update(const SystemState_t *state)
//{
//    if (state == 0)
//    {
//        return;
//    }

//    BSP_Pump_Set(state->pump_on);
//    BSP_Light_Set(state->light_on);
//    BSP_Beep_Set(state->beep_on);
//}

#include "bsp_output.h"
#include "gpio.h"

/* BSP_Output_Init（执行器输出初始化）
 * 作用：系统上电后，先统一关闭所有执行器，避免误动作
 * 说明：当前包括水泵、补光LED、蜂鸣器三个输出
 */
void BSP_Output_Init(void)
{
    /* 关闭水泵 */
    HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_RESET);

    /* 关闭补光LED */
    HAL_GPIO_WritePin(LIGHT_LED_CTRL_GPIO_Port, LIGHT_LED_CTRL_Pin, GPIO_PIN_RESET);

    /* 关闭蜂鸣器 */
    HAL_GPIO_WritePin(BEEP_CTRL_GPIO_Port, BEEP_CTRL_Pin, GPIO_PIN_RESET);
}

/* BSP_Output_Update（执行器状态更新）
 * 作用：根据系统状态结构体中的标志位，统一控制外设输出
 * 参数：state（系统状态结构体指针）
 */
void BSP_Output_Update(const SystemState_t *state)
{
    if (state == 0)
    {
        return;
    }

    /* 水泵控制
     * 说明：
     * 该 MOSFET 驱动模块为高电平启动，
     * 因此 pump_on = 1 时输出高电平，水泵启动；
     * pump_on = 0 时输出低电平，水泵停止。
     */
    if (state->pump_on)
    {
        HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(WATER_PUMP_CTRL_GPIO_Port, WATER_PUMP_CTRL_Pin, GPIO_PIN_RESET);
    }

    /* 补光LED控制 */
    if (state->light_on)
    {
        HAL_GPIO_WritePin(LIGHT_LED_CTRL_GPIO_Port, LIGHT_LED_CTRL_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(LIGHT_LED_CTRL_GPIO_Port, LIGHT_LED_CTRL_Pin, GPIO_PIN_RESET);
    }

    /* 蜂鸣器控制
     * 说明：当前默认按高电平触发方式处理，
     * 若后续实际模块逻辑相反，再统一调整。
     */
    /* 蜂鸣器控制（低电平触发版）
 * 说明：
 * 1. beep_on = 1 时输出低电平，蜂鸣器响
 * 2. beep_on = 0 时输出高电平，蜂鸣器停止
 */
if (state->beep_on)
{
    HAL_GPIO_WritePin(BEEP_CTRL_GPIO_Port, BEEP_CTRL_Pin, GPIO_PIN_RESET);
}
else
{
    HAL_GPIO_WritePin(BEEP_CTRL_GPIO_Port, BEEP_CTRL_Pin, GPIO_PIN_SET);
}
}


