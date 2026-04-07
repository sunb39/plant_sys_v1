//#include "app_control.h"
//#define LIGHT_HYSTERESIS    (20.0f)   /* LIGHT_HYSTERESIS（补光回差值），单位 lux */
///* APP_Control_Init（系统状态初始化）
// * 说明：系统上电时先将所有执行机构关闭，通信状态置为未连接。
// */
//void APP_Control_Init(SystemState_t *state)
//{
//    if (state == 0)
//    {
//        return;
//    }

//    state->pump_on  = 0;
//    state->light_on = 0;
//    state->beep_on  = 0;
//    state->wifi_ok  = 0;
//    state->cloud_ok = 0;
//}

///* APP_Control_Run（控制逻辑运行）
// * 说明：
// * 1. 自动浇水：当土壤湿度低于设定阈值时，开启水泵；
// * 2. 自动补光：当光照低于设定阈值时，开启补光灯；
// * 3. 报警逻辑：当pH超出设定范围时，触发蜂鸣器报警。
// */
//void APP_Control_Run(const SensorData_t *data, const SystemParam_t *param, SystemState_t *state)
//{
//    if ((data == 0) || (param == 0) || (state == 0))
//    {
//        return;
//    }

//    /* 自动浇水逻辑 */
//    if (param->auto_water_en)
//    {
//        if (data->soil_moisture < param->soil_low_th)
//        {
//            state->pump_on = 1;
//        }
//        else
//        {
//            state->pump_on = 0;
//        }
//    }
//    else
//    {
//        state->pump_on = 0;
//    }





///* 自动补光逻辑（带回差）
// * 说明：
// * 1. 当光照值无效（例如 -1.0）时，不更新补光状态；
// * 2. 当光照低于下阈值时，打开补光灯；
// * 3. 当补光灯已经打开后，只有光照高于“下阈值 + 回差值”时才关闭；
// * 4. 这样可以避免在阈值附近反复亮灭。
// */
//if (param->auto_light_en)
//{
//    /* 如果光照值无效，则保持当前补光状态不变 */
//    if (data->light_lux >= 0.0f)
//    {
//        if (state->light_on == 0U)
//        {
//            if (data->light_lux < param->light_low_th)
//            {
//                state->light_on = 1U;
//            }
//        }
//        else
//        {
//            if (data->light_lux > (param->light_low_th + LIGHT_HYSTERESIS))
//            {
//                state->light_on = 0U;
//            }
//        }
//    }
//}
//else
//{
//    state->light_on = 0U;
//}

//    /* 报警逻辑 */
//    if (param->alarm_en)
//    {
//        if ((data->ph_value < param->ph_low_th) || (data->ph_value > param->ph_high_th))
//        {
//            state->beep_on = 1;
//        }
//        else
//        {
//            state->beep_on = 0;
//        }
//    }
//    else
//    {
//        state->beep_on = 0;
//    }
//}

#include "app_control.h"

#define LIGHT_HYSTERESIS    (20.0f)   /* LIGHT_HYSTERESIS（补光回差值），单位 lux */
#define SOIL_HYSTERESIS     (80.0f)   /* SOIL_HYSTERESIS（土壤浇水回差值），单位 ADC 原始值 */
#define PH_HYSTERESIS       (0.10f)   /* PH_HYSTERESIS（pH 报警回差） */

/* APP_Control_Init（系统状态初始化） */
void APP_Control_Init(SystemState_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->pump_on  = 0U;
    state->light_on = 0U;
    state->beep_on  = 0U;
    state->wifi_ok  = 0U;
    state->cloud_ok = 0U;
}

/* APP_Control_Run（控制逻辑运行）
 * 说明：
 * 1. 水泵：根据土壤湿度原始 ADC 值控制
 *    当前 soil_low_th 虽然名字叫 low_th，
 *    但为了兼容旧结构，这里把它当作“土壤干燥阈值”
 *    数值越大表示越干。
 * 2. 补光：根据光照值控制，带回差。
 * 3. 蜂鸣器：用于 pH 异常报警，带回差。
 */
void APP_Control_Run(const SensorData_t *data, const SystemParam_t *param, SystemState_t *state)
{
    if ((data == 0) || (param == 0) || (state == 0))
    {
        return;
    }

    /* =========================
     * 自动浇水逻辑（带回差）
     * 说明：
     * 1. 当前 soil_moisture 存的是 ADC 原始值
     * 2. 数值越大表示越干，越小表示越湿
     * 3. 当原始值高于阈值时，请求浇水
     * =========================
     */
    if (param->auto_water_en)
    {
        if (state->pump_on == 0U)
        {
            /* 当前未浇水：土壤过干则请求开泵 */
            if (data->soil_moisture > param->soil_low_th)
            {
                state->pump_on = 1U;
            }
        }
        else
        {
            /* 当前正在浇水：只有湿度明显回落才停止请求 */
            if (data->soil_moisture < (param->soil_low_th - SOIL_HYSTERESIS))
            {
                state->pump_on = 0U;
            }
        }
    }
    else
    {
        state->pump_on = 0U;
    }

    /* =========================
     * 自动补光逻辑（带回差）
     * 说明：
     * 1. 当光照值无效（例如 -1.0）时，不更新补光状态；
     * 2. 当前 light_low_th 为“补光开启阈值”
     * =========================
     */
    if (param->auto_light_en)
    {
        if (data->light_lux >= 0.0f)
        {
            if (state->light_on == 0U)
            {
                if (data->light_lux < param->light_low_th)
                {
                    state->light_on = 1U;
                }
            }
            else
            {
                if (data->light_lux > (param->light_low_th + LIGHT_HYSTERESIS))
                {
                    state->light_on = 0U;
                }
            }
        }
        /* 光照无效时，保持当前状态不变 */
    }
    else
    {
        state->light_on = 0U;
    }

    /* =========================
     * pH 异常报警逻辑（蜂鸣器）
     * 说明：
     * 1. 蜂鸣器用于 pH 超范围报警
     * 2. 当前不用于浇水提示音
     * =========================
     */
    if (param->alarm_en)
    {
        if (state->beep_on == 0U)
        {
            if ((data->ph_value < param->ph_low_th) || (data->ph_value > param->ph_high_th))
            {
                state->beep_on = 1U;
            }
        }
        else
        {
            if ((data->ph_value > (param->ph_low_th + PH_HYSTERESIS)) &&
                (data->ph_value < (param->ph_high_th - PH_HYSTERESIS)))
            {
                state->beep_on = 0U;
            }
        }
    }
    else
    {
        state->beep_on = 0U;
    }
}

