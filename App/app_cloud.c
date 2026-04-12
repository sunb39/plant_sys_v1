//#include "app_cloud.h"
//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>

///* g_cloud_tx_buf（云端发送缓冲区）
// * 作用：保存最近一次打包好的 OneNET 物模型属性上报 JSON
// */
//static char g_cloud_tx_buf[CLOUD_TX_BUF_SIZE];

///* 这里把土壤 ADC 原始值映射成百分比
// * 说明：
// * 1. 当前 soil_moisture 在您的工程里存的是 ADC 原始值
// * 2. 数值越大越干，越小越湿
// * 3. 这里先给一组经验值，后面可再按实测微调
// */
//#define SOIL_RAW_WET   (1800.0f)   /* 很湿时的 ADC 值 */
//#define SOIL_RAW_DRY   (4200.0f)   /* 很干时的 ADC 值 */

///* APP_Cloud_FormatX10（把 x10 定点数格式化成字符串）
// * 例如：144 -> "14.4"
// */
//static void APP_Cloud_FormatX10(long value_x10, char *out, size_t out_size)
//{
//    long abs_val;

//    if ((out == 0) || (out_size == 0U))
//    {
//        return;
//    }

//    abs_val = (value_x10 >= 0) ? value_x10 : (-value_x10);

//    if (value_x10 < 0)
//    {
//        snprintf(out, out_size, "-%ld.%01ld", abs_val / 10L, abs_val % 10L);
//    }
//    else
//    {
//        snprintf(out, out_size, "%ld.%01ld", abs_val / 10L, abs_val % 10L);
//    }
//}

///* APP_Cloud_FormatX100（把 x100 定点数格式化成字符串）
// * 例如：392 -> "3.92"
// */
//static void APP_Cloud_FormatX100(long value_x100, char *out, size_t out_size)
//{
//    long abs_val;

//    if ((out == 0) || (out_size == 0U))
//    {
//        return;
//    }

//    abs_val = (value_x100 >= 0) ? value_x100 : (-value_x100);

//    if (value_x100 < 0)
//    {
//        snprintf(out, out_size, "-%ld.%02ld", abs_val / 100L, abs_val % 100L);
//    }
//    else
//    {
//        snprintf(out, out_size, "%ld.%02ld", abs_val / 100L, abs_val % 100L);
//    }
//}

///* APP_Cloud_SoilRawToPercent（原始土壤 ADC 转百分比）
// * 说明：
// * 1. 干 -> 0%
// * 2. 湿 -> 100%
// */
//static float APP_Cloud_SoilRawToPercent(float raw)
//{
//    float pct;

//    if (raw <= SOIL_RAW_WET)
//    {
//        return 100.0f;
//    }

//    if (raw >= SOIL_RAW_DRY)
//    {
//        return 0.0f;
//    }

//    pct = (SOIL_RAW_DRY - raw) * 100.0f / (SOIL_RAW_DRY - SOIL_RAW_WET);

//    if (pct < 0.0f)
//    {
//        pct = 0.0f;
//    }

//    if (pct > 100.0f)
//    {
//        pct = 100.0f;
//    }

//    return pct;
//}

///* APP_Cloud_Init（云端通信初始化） */
//void APP_Cloud_Init(SystemState_t *state)
//{
//    if (state != 0)
//    {
//        state->wifi_ok  = 0U;
//        state->cloud_ok = 0U;
//    }

//    memset(g_cloud_tx_buf, 0, sizeof(g_cloud_tx_buf));
//}

///* APP_Cloud_Upload（组织 OneNET 属性上报 JSON）
// * 说明：
// * 1. 按您当前平台功能点标识符输出：
// *    AirTemp / AirHumi / LightLux / SoilMoisture / PHValue
// *    PumpState / LightState / BeepState
// * 2. 这里直接输出 OneJSON 风格，后续 ESP8266 只负责发 topic
// */
//void APP_Cloud_Upload(const SensorData_t *data, const SystemState_t *state)
//{
//    long air_temp_x10;
//    long air_humi_x10;
//    long light_lux_x10;
//    long soil_pct_x10;
//    long ph_x100;

//    char air_temp_str[24];
//    char air_humi_str[24];
//    char light_lux_str[24];
//    char soil_str[24];
//    char ph_str[24];

//    float soil_percent;

//    if ((data == 0) || (state == 0))
//    {
//        return;
//    }

//    air_temp_x10  = (long)(data->air_temp * 10.0f);
//    air_humi_x10  = (long)(data->air_humi * 10.0f);
//    light_lux_x10 = (long)(data->light_lux * 10.0f);

//    soil_percent  = APP_Cloud_SoilRawToPercent(data->soil_moisture);
//    soil_pct_x10  = (long)(soil_percent * 10.0f);

//    ph_x100       = (long)(data->ph_value * 100.0f);

//    APP_Cloud_FormatX10(air_temp_x10,  air_temp_str,  sizeof(air_temp_str));
//    APP_Cloud_FormatX10(air_humi_x10,  air_humi_str,  sizeof(air_humi_str));
//    APP_Cloud_FormatX10(light_lux_x10, light_lux_str, sizeof(light_lux_str));
//    APP_Cloud_FormatX10(soil_pct_x10,  soil_str,      sizeof(soil_str));
//    APP_Cloud_FormatX100(ph_x100,      ph_str,        sizeof(ph_str));

//    snprintf(g_cloud_tx_buf,
//             sizeof(g_cloud_tx_buf),
//             "{"
//             "\"id\":\"123\","
//             "\"version\":\"1.0\","
//             "\"params\":{"
//                 "\"AirTemp\":{\"value\":%s},"
//                 "\"AirHumi\":{\"value\":%s},"
//                 "\"LightLux\":{\"value\":%s},"
//                 "\"SoilMoisture\":{\"value\":%s},"
//                 "\"PHValue\":{\"value\":%s},"
//                 "\"PumpState\":{\"value\":%u},"
//                 "\"LightState\":{\"value\":%u},"
//                 "\"BeepState\":{\"value\":%u}"
//             "}"
//             "}",
//             air_temp_str,
//             air_humi_str,
//             light_lux_str,
//             soil_str,
//             ph_str,
//             state->pump_on,
//             state->light_on,
//             state->beep_on);
//}

///* APP_Cloud_Parse（预留）
// * 说明：本版服务解析放在 bsp_esp8266.c 中做
// */
//void APP_Cloud_Parse(void)
//{
//    /* 预留 */
//}

//const char *APP_Cloud_GetLastPacket(void)
//{
//    return g_cloud_tx_buf;

//}
#include "app_cloud.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* g_cloud_tx_buf（云端发送缓冲区）
 * 作用：保存最近一次打包好的 OneNET 物模型属性上报 JSON
 */
static char g_cloud_tx_buf[CLOUD_TX_BUF_SIZE];

/* 这里把土壤 ADC 原始值映射成百分比 */
#define SOIL_RAW_WET   (1800.0f)   /* 很湿时的 ADC 值 */
#define SOIL_RAW_DRY   (4200.0f)   /* 很干时的 ADC 值 */

/* 把 x10 定点数格式化成字符串 */
static void APP_Cloud_FormatX10(long value_x10, char *out, size_t out_size)
{
    long abs_val;
    if ((out == 0) || (out_size == 0U)) return;
    abs_val = (value_x10 >= 0) ? value_x10 : (-value_x10);
    if (value_x10 < 0) {
        snprintf(out, out_size, "-%ld.%01ld", abs_val / 10L, abs_val % 10L);
    } else {
        snprintf(out, out_size, "%ld.%01ld", abs_val / 10L, abs_val % 10L);
    }
}

/* 把 x100 定点数格式化成字符串 */
static void APP_Cloud_FormatX100(long value_x100, char *out, size_t out_size)
{
    long abs_val;
    if ((out == 0) || (out_size == 0U)) return;
    abs_val = (value_x100 >= 0) ? value_x100 : (-value_x100);
    if (value_x100 < 0) {
        snprintf(out, out_size, "-%ld.%02ld", abs_val / 100L, abs_val % 100L);
    } else {
        snprintf(out, out_size, "%ld.%02ld", abs_val / 100L, abs_val % 100L);
    }
}

/* 原始土壤 ADC 转百分比 */
static float APP_Cloud_SoilRawToPercent(float raw)
{
    float pct;
    if (raw <= SOIL_RAW_WET) return 100.0f;
    if (raw >= SOIL_RAW_DRY) return 0.0f;
    
    pct = (SOIL_RAW_DRY - raw) * 100.0f / (SOIL_RAW_DRY - SOIL_RAW_WET);
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    return pct;
}

/* 云端通信初始化 */
void APP_Cloud_Init(SystemState_t *state)
{
    if (state != 0)
    {
        state->wifi_ok  = 0U;
        state->cloud_ok = 0U;
    }
    memset(g_cloud_tx_buf, 0, sizeof(g_cloud_tx_buf));
}

/* 组织 OneNET 属性上报 JSON */
void APP_Cloud_Upload(const SensorData_t *data, const SystemState_t *state)
{
    long air_temp_x10;
    long air_humi_x10;
    long ph_x100;

    char air_temp_str[24];
    char air_humi_str[24];
    char ph_str[24];

    /* 把光照和土壤改成整数，避免 OneNET 的 double step 步长报错 */
    int light_lux_int;
    int soil_pct_int;
    float soil_percent;

    if ((data == 0) || (state == 0)) return;

    air_temp_x10  = (long)(data->air_temp * 10.0f);
    air_humi_x10  = (long)(data->air_humi * 10.0f);
    ph_x100       = (long)(data->ph_value * 100.0f);

    soil_percent  = APP_Cloud_SoilRawToPercent(data->soil_moisture);
    soil_pct_int  = (int)soil_percent;         /* 强制转为整数 */
    light_lux_int = (int)data->light_lux;      /* 强制转为整数 */

    APP_Cloud_FormatX10(air_temp_x10,  air_temp_str,  sizeof(air_temp_str));
    APP_Cloud_FormatX10(air_humi_x10,  air_humi_str,  sizeof(air_humi_str));
    APP_Cloud_FormatX100(ph_x100,      ph_str,        sizeof(ph_str));

    /* 严谨拼装 JSON，状态值强制限制在 0 或 1 */
    snprintf(g_cloud_tx_buf, sizeof(g_cloud_tx_buf),
             "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{"
             "\"AirTemp\":{\"value\":%s},"
             "\"AirHumi\":{\"value\":%s},"
             "\"LightLux\":{\"value\":%d},"
             "\"SoilMoisture\":{\"value\":%d},"
             "\"PHValue\":{\"value\":%s},"
             "\"PumpState\":{\"value\":%u},"
             "\"LightState\":{\"value\":%u},"
             "\"BeepState\":{\"value\":%u}"
             "}}",
             air_temp_str,
             air_humi_str,
             light_lux_int,
             soil_pct_int,
             ph_str,
             state->pump_on ? 1U : 0U,
             state->light_on ? 1U : 0U,
             state->beep_on ? 1U : 0U);
}

void APP_Cloud_Parse(void)
{
    /* 预留 */
}

const char *APP_Cloud_GetLastPacket(void)
{
    return g_cloud_tx_buf;
}
