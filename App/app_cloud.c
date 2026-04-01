#include "app_cloud.h"
#include <stdio.h>
#include <string.h>

/* g_cloud_tx_buf（云端发送缓冲区）
 * 作用：保存最近一次打包好的上行数据字符串
 */
static char g_cloud_tx_buf[CLOUD_TX_BUF_SIZE];

/* APP_Cloud_Init（云端通信初始化）
 * 说明：当前阶段不做真实ESP8266联网，只完成软件框架初始化。
 */
void APP_Cloud_Init(SystemState_t *state)
{
    if (state != 0)
    {
        state->wifi_ok  = 0;
        state->cloud_ok = 0;
    }

    memset(g_cloud_tx_buf, 0, sizeof(g_cloud_tx_buf));
}

/* APP_Cloud_Upload（云端数据打包）
 * 说明：
 * 1. 当前阶段先完成“数据打包”，后续再接入真实串口发送；
 * 2. 这里用整数放大方式，避免直接使用浮点格式化带来的库配置问题。
 */
void APP_Cloud_Upload(const SensorData_t *data, const SystemState_t *state)
{
    long air_temp_x10;
    long air_humi_x10;
    long light_lux_x10;
    long soil_x10;
    long ph_x100;

    if ((data == 0) || (state == 0))
    {
        return;
    }

    air_temp_x10  = (long)(data->air_temp * 10.0f);
    air_humi_x10  = (long)(data->air_humi * 10.0f);
    light_lux_x10 = (long)(data->light_lux * 10.0f);
    soil_x10      = (long)(data->soil_moisture * 10.0f);
    ph_x100       = (long)(data->ph_value * 100.0f);

    /* 打包为类 JSON 格式字符串
     * 答辩可说：当前先完成协议字段组织，后续只需把该字符串通过串口发给 ESP8266 即可。
     */
    snprintf(g_cloud_tx_buf,
             sizeof(g_cloud_tx_buf),
             "{\"air_temp_x10\":%ld,"
             "\"air_humi_x10\":%ld,"
             "\"light_lux_x10\":%ld,"
             "\"soil_moisture_x10\":%ld,"
             "\"ph_value_x100\":%ld,"
             "\"pump_on\":%u,"
             "\"light_on\":%u,"
             "\"beep_on\":%u}",
             air_temp_x10,
             air_humi_x10,
             light_lux_x10,
             soil_x10,
             ph_x100,
             state->pump_on,
             state->light_on,
             state->beep_on);
}

/* APP_Cloud_Parse（云端报文解析）
 * 说明：当前阶段先预留接口，后续接入串口接收缓冲区后再做解析。
 */
void APP_Cloud_Parse(void)
{
    /* 以后可在这里解析类似：
     * "PUMP=1"
     * "LIGHT=0"
     * "AUTO_WATER=1"
     * 等控制命令
     */
}

/* APP_Cloud_GetLastPacket（获取最近一次待上传报文）
 * 说明：便于后续串口调试、界面显示或日志打印。
 */
const char *APP_Cloud_GetLastPacket(void)
{
    return g_cloud_tx_buf;
}
