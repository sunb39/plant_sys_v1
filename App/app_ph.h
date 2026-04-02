#ifndef __APP_PH_H
#define __APP_PH_H

#include <stdint.h>

/* PHParam_t（pH 换算参数结构体）
 * 作用：
 * 统一保存 pH 计算相关参数，包括 ADC 参考电压、ADC 最大计数、
 * 以及线性拟合公式参数 k 和 b。
 *
 * 答辩可说：
 * 为了提高软件可扩展性，pH 模块采用“参数化换算”的设计方法，
 * 将 ADC 到电压、电压到 pH 的关系独立封装，便于后续标定修正。
 */
typedef struct
{
    float adc_vref;       /* adc_vref（ADC 参考电压） */
    uint16_t adc_max;     /* adc_max（ADC 最大计数值） */
    float k;              /* k（线性拟合斜率） */
    float b;              /* b（线性拟合截距） */
} PHParam_t;

/* APP_PH_Init（pH 模块软件参数初始化）
 * 作用：加载默认 ADC 参数与默认拟合公式
 */
void APP_PH_Init(void);

/* APP_PH_SetAdcVref（设置 ADC 参考电压）
 * 作用：后续如果换 ADC 或换采样电压体系，可单独修改参考电压
 */
void APP_PH_SetAdcVref(float adc_vref);

/* APP_PH_SetLinearParam（设置 pH 线性拟合参数）
 * 作用：手动设置拟合斜率和截距
 */
void APP_PH_SetLinearParam(float k, float b);

/* APP_PH_CalibrateByTwoPoint（基于两点进行标定）
 * 作用：根据两组“电压-标准 pH 值”计算新的拟合参数
 *
 * 参数说明：
 * voltage1：第一点电压
 * ph1：第一点标准 pH
 * voltage2：第二点电压
 * ph2：第二点标准 pH
 */
void APP_PH_CalibrateByTwoPoint(float voltage1, float ph1,
                                float voltage2, float ph2);

/* APP_PH_AdcToVoltage（ADC 原始值转电压）
 * 作用：将 12 位 ADC 采样值转换为实际电压值
 */
float APP_PH_AdcToVoltage(uint16_t adc_value);

/* APP_PH_VoltageToValue（电压转 pH 值）
 * 作用：根据拟合公式把电压换算成 pH
 */
float APP_PH_VoltageToValue(float voltage);

/* APP_PH_GetValue（ADC 原始值直接转 pH 值）
 * 作用：对上层提供最直接的接口
 */
float APP_PH_GetValue(uint16_t adc_value);

/* APP_PH_GetParam（获取当前 pH 参数）
 * 作用：便于调试输出、界面显示或后续保存到 Flash
 */
void APP_PH_GetParam(PHParam_t *param);

#endif
