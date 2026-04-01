#include "app_sensor.h"
#include "plant_config.h"

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
 * 1. 当前 USE_MOCK_DATA=1，使用模拟值驱动整个系统的软件逻辑。
 * 2. 后续硬件到位后，只需要把 #else 部分替换为真实采集即可。
 */
void APP_Sensor_Update(SensorData_t *data)
{
    if (data == 0)
    {
        return;
    }

#if USE_MOCK_DATA
    /* static（静态变量）
     * 作用：让模拟值在多次调用之间保持变化趋势，
     * 模拟真实环境数据的动态波动。
     */
    static float temp = 25.0f;
    static float humi = 60.0f;
    static float lux  = 300.0f;
    static float soil = 45.0f;
    static float ph   = 6.5f;

    /* 模拟空气温度变化 */
    temp += 0.1f;
    if (temp > 30.0f)
    {
        temp = 25.0f;
    }

    /* 模拟空气湿度变化 */
    humi += 0.2f;
    if (humi > 75.0f)
    {
        humi = 60.0f;
    }

    /* 模拟光照变化 */
    lux += 20.0f;
    if (lux > 800.0f)
    {
        lux = 300.0f;
    }

    /* 模拟土壤湿度变化
     * 这里设计成逐渐下降，便于观察“自动浇水”逻辑是否会触发。
     */
    soil -= 0.3f;
    if (soil < 20.0f)
    {
        soil = 45.0f;
    }

    /* 模拟 pH 变化 */
    ph += 0.02f;
    if (ph > 7.5f)
    {
        ph = 6.5f;
    }

    /* 将模拟值写入结构体 */
    data->air_temp      = temp;
    data->air_humi      = humi;
    data->light_lux     = lux;
    data->soil_moisture = soil;
    data->ph_value      = ph;

#else
    /* 真实硬件采集区（后续替换）
     * 例如：
     * data->air_temp      = BSP_DHT11_ReadTemp();
     * data->air_humi      = BSP_DHT11_ReadHumi();
     * data->light_lux     = BSP_BH1750_ReadLux();
     * data->soil_moisture = BSP_ADC_ReadSoil();
     * data->ph_value      = BSP_ADC_ReadPH();
     */
#endif
}
