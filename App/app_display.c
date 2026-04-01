#include "app_display.h"
#include <stdio.h>
#include <string.h>

/* g_display_cache（显示缓存）
 * 作用：保存当前页面要显示的4行文本内容
 * 当前阶段不接 OLED 时，也可用于串口查看页面组织是否正确。
 */
static DisplayCache_t g_display_cache;

/* APP_Display_ClearCache（清空显示缓存）
 * 说明：每次更新页面前先清空，避免残留旧内容。
 */
static void APP_Display_ClearCache(void)
{
    memset(&g_display_cache, 0, sizeof(g_display_cache));
}

/* APP_Display_Init（显示模块初始化） */
void APP_Display_Init(void)
{
    APP_Display_ClearCache();
}

/* APP_Display_Show（显示页面组织）
 * 说明：
 * 1. 当前阶段先把不同页面的数据整理成4行字符串；
 * 2. 后续接入 OLED 后，只需要把这4行字符串显示到屏幕即可。
 */
void APP_Display_Show(PageId_t page,
                      const SensorData_t *data,
                      const SystemParam_t *param,
                      const SystemState_t *state)
{
    if ((data == 0) || (param == 0) || (state == 0))
    {
        return;
    }

    APP_Display_ClearCache();

    switch (page)
    {
        case PAGE_MAIN:
            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "Smart Plant Sys");
            snprintf(g_display_cache.line2, sizeof(g_display_cache.line2), "T:%.1f H:%.1f", data->air_temp, data->air_humi);
            snprintf(g_display_cache.line3, sizeof(g_display_cache.line3), "Soil:%.1f Lux:%.1f", data->soil_moisture, data->light_lux);
            snprintf(g_display_cache.line4, sizeof(g_display_cache.line4), "P:%d L:%d B:%d", state->pump_on, state->light_on, state->beep_on);
            break;

        case PAGE_SENSOR:
            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "Sensor Data");
            snprintf(g_display_cache.line2, sizeof(g_display_cache.line2), "Temp: %.1f C", data->air_temp);
            snprintf(g_display_cache.line3, sizeof(g_display_cache.line3), "Humi: %.1f %%", data->air_humi);
            snprintf(g_display_cache.line4, sizeof(g_display_cache.line4), "Lux:%.1f pH:%.2f", data->light_lux, data->ph_value);
            break;

        case PAGE_PARAM:
            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "Param Config");
            snprintf(g_display_cache.line2, sizeof(g_display_cache.line2), "SoilTH: %.1f", param->soil_low_th);
            snprintf(g_display_cache.line3, sizeof(g_display_cache.line3), "LightTH: %.1f", param->light_low_th);
            snprintf(g_display_cache.line4, sizeof(g_display_cache.line4), "pH: %.1f~%.1f", param->ph_low_th, param->ph_high_th);
            break;

        case PAGE_STATE:
            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "System State");
            snprintf(g_display_cache.line2, sizeof(g_display_cache.line2), "Pump:%d Light:%d", state->pump_on, state->light_on);
            snprintf(g_display_cache.line3, sizeof(g_display_cache.line3), "Beep:%d WiFi:%d", state->beep_on, state->wifi_ok);
            snprintf(g_display_cache.line4, sizeof(g_display_cache.line4), "Cloud:%d", state->cloud_ok);
            break;

        default:
            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "Unknown Page");
            break;
    }

    /* 后续接入 OLED 时，可在这里调用类似：
     * OLED_ShowString(0, 0, g_display_cache.line1);
     * OLED_ShowString(0, 2, g_display_cache.line2);
     * OLED_ShowString(0, 4, g_display_cache.line3);
     * OLED_ShowString(0, 6, g_display_cache.line4);
     */
}

/* APP_Display_GetCache（获取显示缓存）
 * 作用：供调试打印或后续显示驱动调用
 */
const DisplayCache_t *APP_Display_GetCache(void)
{
    return &g_display_cache;
}
