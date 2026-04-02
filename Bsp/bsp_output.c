#include "bsp_output.h"

/* BSP_Output_Write（底层输出辅助函数）
 * 作用：
 * 根据“有效电平”自动转换 GPIO 输出状态
 * 这样以后如果模块是低电平触发，只改宏定义即可，不用改函数逻辑
 */
static void BSP_Output_Write(GPIO_TypeDef *port,
                             uint16_t pin,
                             GPIO_PinState active_level,
                             GPIO_PinState inactive_level,
                             uint8_t on)
{
    if (on)
    {
        HAL_GPIO_WritePin(port, pin, active_level);
    }
    else
    {
        HAL_GPIO_WritePin(port, pin, inactive_level);
    }
}

/* BSP_Output_Init（执行器输出初始化）
 * 说明：系统上电默认全部关闭，避免误动作
 */
void BSP_Output_Init(void)
{
    BSP_Pump_Set(0);
    BSP_Light_Set(0);
    BSP_Beep_Set(0);
}

/* BSP_Pump_Set（水泵开关控制）
 * 说明：你的 MOSFET 水泵驱动资料写的是“高电平启动”
 */
void BSP_Pump_Set(uint8_t on)
{
    BSP_Output_Write(PUMP_GPIO_PORT,
                     PUMP_GPIO_PIN,
                     PUMP_ACTIVE_LEVEL,
                     PUMP_INACTIVE_LEVEL,
                     on);
}

/* BSP_Light_Set（补光输出控制） */
void BSP_Light_Set(uint8_t on)
{
    BSP_Output_Write(LIGHT_GPIO_PORT,
                     LIGHT_GPIO_PIN,
                     LIGHT_ACTIVE_LEVEL,
                     LIGHT_INACTIVE_LEVEL,
                     on);
}

/* BSP_Beep_Set（蜂鸣器控制）
 * 说明：如果后面你实物是低电平触发蜂鸣器，只需要改 bsp_board.h 里的有效电平宏
 */
void BSP_Beep_Set(uint8_t on)
{
    BSP_Output_Write(BEEP_GPIO_PORT,
                     BEEP_GPIO_PIN,
                     BEEP_ACTIVE_LEVEL,
                     BEEP_INACTIVE_LEVEL,
                     on);
}

/* BSP_Output_Update（根据系统状态统一更新输出）
 * 说明：
 * 这个函数建议放在主循环中调用，
 * 这样应用层只负责“判断状态”，输出层只负责“执行动作”。
 */
void BSP_Output_Update(const SystemState_t *state)
{
    if (state == 0)
    {
        return;
    }

    BSP_Pump_Set(state->pump_on);
    BSP_Light_Set(state->light_on);
    BSP_Beep_Set(state->beep_on);
}
