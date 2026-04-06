#include "app_control.h"
#define LIGHT_HYSTERESIS    (20.0f)   /* LIGHT_HYSTERESIS（补光回差值），单位 lux */
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


    /* 自动补光逻辑（带回差）
 * 说明：
 * 1. 当光照低于下阈值时，打开补光灯；
 * 2. 当补光灯已经打开后，只有光照高于“下阈值 + 回差值”时才关闭；
 * 3. 这样可以避免在阈值附近反复亮灭。
 */
if (param->auto_light_en)
{
    /* 如果当前补光灯是关闭状态 */
    if (state->light_on == 0U)
    {
        /* 光照低于下阈值，打开补光灯 */
        if (data->light_lux < param->light_low_th)
        {
            state->light_on = 1U;
        }
    }
    else
    {
        /* 如果当前补光灯已经打开，
         * 只有当光照高于“下阈值 + 回差值”时才关闭
         */
        if (data->light_lux > (param->light_low_th + LIGHT_HYSTERESIS))
        {
            state->light_on = 0U;
        }
    }
}
else
{
    /* 自动补光功能关闭时，强制关闭补光灯 */
    state->light_on = 0U;
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
