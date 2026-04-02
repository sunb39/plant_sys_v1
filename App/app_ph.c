#include "app_ph.h"
#include "plant_config.h"

/* g_ph_param（全局 pH 参数）
 * 作用：保存当前正在使用的 pH 换算参数
 */
static PHParam_t g_ph_param;

/* APP_PH_Limit（限幅函数）
 * 作用：把结果限制在指定区间内，避免出现异常值
 */
static float APP_PH_Limit(float value, float min_val, float max_val)
{
    if (value < min_val)
    {
        return min_val;
    }

    if (value > max_val)
    {
        return max_val;
    }

    return value;
}

/* APP_PH_Init（pH 模块软件参数初始化）
 * 说明：
 * 1. ADC 参考电压默认按 3.3V
 * 2. ADC 最大计数默认按 12 位 4095
 * 3. 线性拟合参数先按你资料中的 3.3V ADC 曲线设定
 */
void APP_PH_Init(void)
{
    g_ph_param.adc_vref = PH_ADC_VREF;
    g_ph_param.adc_max  = PH_ADC_MAX_VALUE;
    g_ph_param.k        = PH_DEFAULT_K;
    g_ph_param.b        = PH_DEFAULT_B;
}

/* APP_PH_SetAdcVref（设置 ADC 参考电压） */
void APP_PH_SetAdcVref(float adc_vref)
{
    if (adc_vref > 0.0f)
    {
        g_ph_param.adc_vref = adc_vref;
    }
}

/* APP_PH_SetLinearParam（设置线性拟合参数） */
void APP_PH_SetLinearParam(float k, float b)
{
    g_ph_param.k = k;
    g_ph_param.b = b;
}

/* APP_PH_CalibrateByTwoPoint（两点标定）
 * 说明：
 * 已知：
 *   ph = k * voltage + b
 * 又已知两点：
 *   ph1 = k * voltage1 + b
 *   ph2 = k * voltage2 + b
 * 可求得：
 *   k = (ph2 - ph1) / (voltage2 - voltage1)
 *   b = ph1 - k * voltage1
 */
void APP_PH_CalibrateByTwoPoint(float voltage1, float ph1,
                                float voltage2, float ph2)
{
    float k;
    float b;

    /* 防止除零 */
    if ((voltage2 - voltage1) == 0.0f)
    {
        return;
    }

    k = (ph2 - ph1) / (voltage2 - voltage1);
    b = ph1 - (k * voltage1);

    g_ph_param.k = k;
    g_ph_param.b = b;
}

/* APP_PH_AdcToVoltage（ADC 原始值转电压）
 * 说明：
 * voltage = adc_value / adc_max * adc_vref
 */
float APP_PH_AdcToVoltage(uint16_t adc_value)
{
    float voltage;

    voltage = ((float)adc_value * g_ph_param.adc_vref) / (float)g_ph_param.adc_max;
    return voltage;
}

/* APP_PH_VoltageToValue（电压转 pH）
 * 说明：
 * 默认公式：
 * pH = k * voltage + b
 */
float APP_PH_VoltageToValue(float voltage)
{
    float ph_value;

    ph_value = (g_ph_param.k * voltage) + g_ph_param.b;

    /* 限制 pH 值在 0~14 之间 */
    ph_value = APP_PH_Limit(ph_value, PH_MIN_VALUE, PH_MAX_VALUE);

    return ph_value;
}

/* APP_PH_GetValue（ADC 原始值直接转 pH）
 * 说明：
 * 先把 ADC 转成电压，再把电压转成 pH
 */
float APP_PH_GetValue(uint16_t adc_value)
{
    float voltage;
    float ph_value;

    voltage  = APP_PH_AdcToVoltage(adc_value);
    ph_value = APP_PH_VoltageToValue(voltage);

    return ph_value;
}

/* APP_PH_GetParam（获取当前 pH 参数） */
void APP_PH_GetParam(PHParam_t *param)
{
    if (param == 0)
    {
        return;
    }

    *param = g_ph_param;
}
