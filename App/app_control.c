#include "app_control.h"

/* APP_Control_Init（系统状态初始化）
 * 说明：系统上电时先将所有执行机构关闭，通信状态置为未连接。
 */
void APP_Control_Init(SystemState_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->pump_on  = 0;
    state->light_on = 0;
    state->beep_on  = 0;
    state->wifi_ok  = 0;
    state->cloud_ok = 0;
}

/* APP_Control_Run（控制逻辑运行）
 * 说明：
 * 1. 自动浇水：当土壤湿度低于设定阈值时，开启水泵；
 * 2. 自动补光：当光照低于设定阈值时，开启补光灯；
 * 3. 报警逻辑：当pH超出设定范围时，触发蜂鸣器报警。
 */
void APP_Control_Run(const SensorData_t *data, const SystemParam_t *param, SystemState_t *state)
{
    if ((data == 0) || (param == 0) || (state == 0))
    {
        return;
    }

    /* 自动浇水逻辑 */
    if (param->auto_water_en)
    {
        if (data->soil_moisture < param->soil_low_th)
        {
            state->pump_on = 1;
        }
        else
        {
            state->pump_on = 0;
        }
    }
    else
    {
        state->pump_on = 0;
    }

    /* 自动补光逻辑 */
    if (param->auto_light_en)
    {
        if (data->light_lux < param->light_low_th)
        {
            state->light_on = 1;
        }
        else
        {
            state->light_on = 0;
        }
    }
    else
    {
        state->light_on = 0;
    }

    /* 报警逻辑 */
    if (param->alarm_en)
    {
        if ((data->ph_value < param->ph_low_th) || (data->ph_value > param->ph_high_th))
        {
            state->beep_on = 1;
        }
        else
        {
            state->beep_on = 0;
        }
    }
    else
    {
        state->beep_on = 0;
    }
}
