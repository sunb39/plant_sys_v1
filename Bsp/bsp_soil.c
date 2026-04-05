#include "bsp_soil.h"
#include "adc.h"

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
    uint32_t sum = 0;
    uint32_t value1 = 0;
    uint32_t value2 = 0;
    uint8_t i = 0;

    if (raw == 0)
    {
        return HAL_ERROR;
    }

    for (i = 0; i < 8; i++)
    {
        if (HAL_ADC_Start(&hadc1) != HAL_OK)
        {
            return HAL_ERROR;
        }

        /* 第1个转换：PA0 土壤湿度 */
        if (HAL_ADC_PollForConversion(&hadc1, 20) != HAL_OK)
        {
            HAL_ADC_Stop(&hadc1);
            return HAL_ERROR;
        }
        value1 = HAL_ADC_GetValue(&hadc1);

        /* 第2个转换：PA1 pH，当前先丢掉 */
        if (HAL_ADC_PollForConversion(&hadc1, 20) != HAL_OK)
        {
            HAL_ADC_Stop(&hadc1);
            return HAL_ERROR;
        }
        value2 = HAL_ADC_GetValue(&hadc1);
        (void)value2;

        HAL_ADC_Stop(&hadc1);

        sum += value1;
        HAL_Delay(2);
    }

    *raw = (uint16_t)(sum / 8);
    return HAL_OK;
}

