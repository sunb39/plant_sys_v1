#include "app_sensor.h"
#include "plant_config.h"
#include "bsp_bh1750.h"
#include "bsp_dht11.h"
#include "bsp_debug.h"
#include "bsp_soil.h"
#include "app_ph.h"
/* APP_Sensor_Init（传感器数据初始化）
 * 说明：给传感器结构体一个默认值，便于系统上电后界面有正常初值
 */
void APP_Sensor_Init(SensorData_t *data)
{
    if (data == 0)
    {
        return;
    }

    data->air_temp      = 25.0f;
    data->air_humi      = 60.0f;
    data->light_lux     = 300.0f;
    data->soil_moisture = 45.0f;
    data->ph_value      = 6.5f;
}

/* APP_Sensor_Update（传感器数据更新）
 * 说明：
 * 1. 各传感器分别由独立 mock 开关控制
 * 2. 哪一路硬件调通，就把对应 mock 关闭
 */
void APP_Sensor_Update(SensorData_t *data)
{
    if (data == 0)
    {
        return;
    }

#if USE_MOCK_DHT11
    static float temp = 25.0f;
    static float humi = 60.0f;
#endif

#if USE_MOCK_SOIL
    static float soil = 45.0f;
#endif

#if USE_MOCK_PH
    ph += 0.02f;
    if (ph > 7.5f)
    {
        ph = 6.5f;
    }
    data->ph_value = ph;
#else
    {
        uint16_t ph_raw = 0U;   /* ph_raw（pH 原始 ADC 值） */

        if (BSP_PH_ReadRaw(&ph_raw) == HAL_OK)
        {
            /* 直接把 ADC 原始值换算成 pH */
            data->ph_value = APP_PH_GetValue(ph_raw);
        }
        /* 失败时保留上一次有效值 */
    }
#endif

#if USE_MOCK_DHT11
    static float temp = 25.0f;
    static float humi = 60.0f;

    temp += 0.1f;
    if (temp > 30.0f)
    {
        temp = 25.0f;
    }

    humi += 0.2f;
    if (humi > 75.0f)
    {
        humi = 60.0f;
    }

    data->air_temp = temp;
    data->air_humi = humi;
#else
    {
        static uint32_t dht11_tick = 0U;
        float temp_real = 0.0f;
        float humi_real = 0.0f;
        uint32_t now_tick = HAL_GetTick();

        /* DHT11 不要高频读取，建议 2 秒读一次 */
        if ((now_tick - dht11_tick) >= DHT11_PERIOD_MS)
        {
            dht11_tick = now_tick;

            if (BSP_DHT11_Read(&temp_real, &humi_real) == HAL_OK)
            {
                data->air_temp = temp_real;
                data->air_humi = humi_real;

                BSP_Debug_Printf("DHT11 OK: T=%.1f H=%.1f\r\n",
                                 data->air_temp,
                                 data->air_humi);
            }
            else
            {
                BSP_Debug_Printf("DHT11 Read Fail\r\n");
                /* 失败时保留上一次有效值 */
            }
        }
    }
#endif

#if USE_MOCK_BH1750
    {
        static float lux = 300.0f;

        lux += 20.0f;
        if (lux > 800.0f)
        {
            lux = 300.0f;
        }
        data->light_lux = lux;
    }
#else
    if (BSP_BH1750_ReadLux(&data->light_lux) != HAL_OK)
    {
        data->light_lux = 0.0f;
    }
#endif

#if USE_MOCK_SOIL
    static float soil = 45.0f;

    soil -= 0.3f;
    if (soil < 20.0f)
    {
        soil = 45.0f;
    }
    data->soil_moisture = soil;
#else
    {
        uint16_t soil_raw = 0U;

        if (BSP_Soil_ReadRaw(&soil_raw) == HAL_OK)
        {
            /* 第一阶段先显示 ADC 原始值 */
            data->soil_moisture = (float)soil_raw;
        }
        /* 失败时保留上一次有效值 */
    }
#endif

#if USE_MOCK_PH
    ph += 0.02f;
    if (ph > 7.5f)
    {
        ph = 6.5f;
    }
    data->ph_value = ph;
#else
    /* 这里后面接入真实 pH ADC */
#endif
}
