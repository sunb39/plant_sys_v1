#include "bsp_soil.h"
#include "adc.h"

/* BSP_ADC_ReadDualAverage（双通道平均采样）
 * 作用：
 * 1. 连续读取 ADC1 双通道扫描结果
 * 2. 第1通道 PA0 = 土壤湿度
 * 3. 第2通道 PA1 = pH
 * 4. 进行 8 次平均，降低抖动
 */
static HAL_StatusTypeDef BSP_ADC_ReadDualAverage(uint16_t *soil_raw, uint16_t *ph_raw)
{
    uint32_t sum_soil = 0U;
    uint32_t sum_ph   = 0U;
    uint32_t value1   = 0U;
    uint32_t value2   = 0U;
    uint8_t i = 0U;

    if ((soil_raw == 0) && (ph_raw == 0))
    {
        return HAL_ERROR;
    }

    for (i = 0U; i < 8U; i++)
    {
        if (HAL_ADC_Start(&hadc1) != HAL_OK)
        {
            return HAL_ERROR;
        }

        /* 第1个转换：PA0 土壤湿度 */
        if (HAL_ADC_PollForConversion(&hadc1, 20U) != HAL_OK)
        {
            HAL_ADC_Stop(&hadc1);
            return HAL_ERROR;
        }
        value1 = HAL_ADC_GetValue(&hadc1);

        /* 第2个转换：PA1 pH */
        if (HAL_ADC_PollForConversion(&hadc1, 20U) != HAL_OK)
        {
            HAL_ADC_Stop(&hadc1);
            return HAL_ERROR;
        }
        value2 = HAL_ADC_GetValue(&hadc1);

        HAL_ADC_Stop(&hadc1);

        sum_soil += value1;
        sum_ph   += value2;

        HAL_Delay(2U);
    }

    if (soil_raw != 0)
    {
        *soil_raw = (uint16_t)(sum_soil / 8U);
    }

    if (ph_raw != 0)
    {
        *ph_raw = (uint16_t)(sum_ph / 8U);
    }

    return HAL_OK;
}

HAL_StatusTypeDef BSP_Soil_Init(void)
{
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef BSP_Soil_ReadRaw(uint16_t *raw)
{
    return BSP_ADC_ReadDualAverage(raw, 0);
}

HAL_StatusTypeDef BSP_PH_ReadRaw(uint16_t *raw)
{
    return BSP_ADC_ReadDualAverage(0, raw);
}

//#include "bsp_soil.h"
//#include "adc.h"

///* BSP_ADC_ReadDualAverage（双通道平均采样）
// * 作用：
// * 1. 连续读取 ADC1 双通道扫描结果多次
// * 2. 第1通道为 PA0（土壤湿度）
// * 3. 第2通道为 PA1（pH）
// * 4. 对结果做简单平均，降低抖动
// */
//static HAL_StatusTypeDef BSP_ADC_ReadDualAverage(uint16_t *soil_raw, uint16_t *ph_raw)
//{
//    uint32_t sum_soil = 0U;   /* sum_soil（土壤原始值累加和） */
//    uint32_t sum_ph   = 0U;   /* sum_ph（pH原始值累加和） */
//    uint32_t value1   = 0U;   /* value1（第1通道转换结果，PA0） */
//    uint32_t value2   = 0U;   /* value2（第2通道转换结果，PA1） */
//    uint8_t  i        = 0U;   /* i（循环计数） */

//    /* 至少要有一个输出指针有效 */
//    if ((soil_raw == 0) && (ph_raw == 0))
//    {
//        return HAL_ERROR;
//    }

//    for (i = 0U; i < 8U; i++)
//    {
//        /* 启动一次 ADC 扫描转换 */
//        if (HAL_ADC_Start(&hadc1) != HAL_OK)
//        {
//            return HAL_ERROR;
//        }

//        /* 第1个转换结果：PA0 = 土壤湿度 */
//        if (HAL_ADC_PollForConversion(&hadc1, 20U) != HAL_OK)
//        {
//            HAL_ADC_Stop(&hadc1);
//            return HAL_ERROR;
//        }
//        value1 = HAL_ADC_GetValue(&hadc1);

//        /* 第2个转换结果：PA1 = pH */
//        if (HAL_ADC_PollForConversion(&hadc1, 20U) != HAL_OK)
//        {
//            HAL_ADC_Stop(&hadc1);
//            return HAL_ERROR;
//        }
//        value2 = HAL_ADC_GetValue(&hadc1);

//        HAL_ADC_Stop(&hadc1);

//        sum_soil += value1;
//        sum_ph   += value2;

//        HAL_Delay(2U);
//    }

//    if (soil_raw != 0)
//    {
//        *soil_raw = (uint16_t)(sum_soil / 8U);
//    }

//    if (ph_raw != 0)
//    {
//        *ph_raw = (uint16_t)(sum_ph / 8U);
//    }

//    return HAL_OK;
//}

///* BSP_Soil_Init（土壤湿度 ADC 初始化）
// * 说明：
// * 这里做一次 ADC 校准
// */
//HAL_StatusTypeDef BSP_Soil_Init(void)
//{
//    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    return HAL_OK;
//}

///* BSP_Soil_ReadRaw（读取土壤湿度原始值） */
//HAL_StatusTypeDef BSP_Soil_ReadRaw(uint16_t *raw)
//{
//    return BSP_ADC_ReadDualAverage(raw, 0);
//}

///* BSP_PH_ReadRaw（读取 pH 原始值） */
//HAL_StatusTypeDef BSP_PH_ReadRaw(uint16_t *raw)
//{
//    return BSP_ADC_ReadDualAverage(0, raw);
//}

