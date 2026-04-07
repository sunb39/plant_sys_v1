//#include "bsp_soil.h"
//#include "adc.h"

///* BSP_ADC_ReadDualAverage（双通道平均采样）
// * 作用：
// * 1. 连续读取 ADC1 双通道扫描结果
// * 2. 第1通道 PA0 = 土壤湿度
// * 3. 第2通道 PA1 = pH
// * 4. 进行 8 次平均，降低抖动
// */
//static HAL_StatusTypeDef BSP_ADC_ReadDualAverage(uint16_t *soil_raw, uint16_t *ph_raw)
//{
//    uint32_t sum_soil = 0U;
//    uint32_t sum_ph   = 0U;
//    uint32_t value1   = 0U;
//    uint32_t value2   = 0U;
//    uint8_t i = 0U;

//    if ((soil_raw == 0) && (ph_raw == 0))
//    {
//        return HAL_ERROR;
//    }

//    for (i = 0U; i < 8U; i++)
//    {
//        if (HAL_ADC_Start(&hadc1) != HAL_OK)
//        {
//            return HAL_ERROR;
//        }

//        /* 第1个转换：PA0 土壤湿度 */
//        if (HAL_ADC_PollForConversion(&hadc1, 20U) != HAL_OK)
//        {
//            HAL_ADC_Stop(&hadc1);
//            return HAL_ERROR;
//        }
//        value1 = HAL_ADC_GetValue(&hadc1);

//        /* 第2个转换：PA1 pH */
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

//HAL_StatusTypeDef BSP_Soil_Init(void)
//{
//    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }
//    return HAL_OK;
//}

//HAL_StatusTypeDef BSP_Soil_ReadRaw(uint16_t *raw)
//{
//    return BSP_ADC_ReadDualAverage(raw, 0);
//}

//HAL_StatusTypeDef BSP_PH_ReadRaw(uint16_t *raw)
//{
//    return BSP_ADC_ReadDualAverage(0, raw);
//}


#include "bsp_soil.h"
#include "adc.h"

/* BSP_ADC_ReadSingleAverage（单通道平均采样）
 * 作用：
 * 1. 动态选择一个 ADC 通道
 * 2. 连续采样 8 次并平均
 * 3. 避免 CH0 / CH1 在扫描模式下互相串扰
 */
static HAL_StatusTypeDef BSP_ADC_ReadSingleAverage(uint32_t channel, uint16_t *raw)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint32_t sum = 0U;
    uint32_t value = 0U;
    uint8_t i = 0U;

    if (raw == 0)
    {
        return HAL_ERROR;
    }

    /* 动态切换当前采样通道 */
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        return HAL_ERROR;
    }

    for (i = 0U; i < 8U; i++)
    {
        if (HAL_ADC_Start(&hadc1) != HAL_OK)
        {
            return HAL_ERROR;
        }

        if (HAL_ADC_PollForConversion(&hadc1, 20U) != HAL_OK)
        {
            HAL_ADC_Stop(&hadc1);
            return HAL_ERROR;
        }

        value = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);

        sum += value;
        HAL_Delay(2U);
    }

    *raw = (uint16_t)(sum / 8U);
    return HAL_OK;
}

/* BSP_Soil_Init（土壤湿度 / pH ADC 初始化） */
HAL_StatusTypeDef BSP_Soil_Init(void)
{
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* BSP_Soil_ReadRaw（读取土壤湿度原始值）
 * 说明：
 * 读取 PA0 = ADC_CHANNEL_0
 */
HAL_StatusTypeDef BSP_Soil_ReadRaw(uint16_t *raw)
{
    return BSP_ADC_ReadSingleAverage(ADC_CHANNEL_0, raw);
}

/* BSP_PH_ReadRaw（读取 pH 原始值）
 * 说明：
 * 读取 PA1 = ADC_CHANNEL_1
 */
HAL_StatusTypeDef BSP_PH_ReadRaw(uint16_t *raw)
{
    return BSP_ADC_ReadSingleAverage(ADC_CHANNEL_1, raw);
}
