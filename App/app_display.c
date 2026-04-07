//#include "app_display.h"
//#include "bsp_oled.h"
//#include <stdio.h>
//#include <string.h>

///* g_display_cache（显示缓存）
// * 作用：保存当前页面要显示的4行文本内容
// * 当前阶段既可用于串口查看，也可用于 OLED 显示
// */
//static DisplayCache_t g_display_cache;

///* APP_Display_ClearCache（清空显示缓存） */
//static void APP_Display_ClearCache(void)
//{
//    memset(&g_display_cache, 0, sizeof(g_display_cache));
//}

///* APP_Display_Init（显示模块初始化） */
//void APP_Display_Init(void)
//{
//    APP_Display_ClearCache();
//}

///* APP_Display_Show（显示页面组织并刷新到 OLED）
// * 说明：
// * 1. 先组织4行页面缓存
// * 2. 再通过 BSP_OLED_ShowString() 显示到 OLED
// * 3. 当前页面文字统一采用大写，便于匹配简化字库
// */
//void APP_Display_Show(PageId_t page,
//                      const SensorData_t *data,
//                      const SystemParam_t *param,
//                      const SystemState_t *state)
//{
//    if ((data == 0) || (param == 0) || (state == 0))
//    {
//        return;
//    }

//    APP_Display_ClearCache();

//    switch (page)
//    {
//        case PAGE_MAIN:
//            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "PLANT SYS");
//            snprintf(g_display_cache.line2, sizeof(g_display_cache.line2), "T:%.1f H:%.1f", data->air_temp, data->air_humi);
//            snprintf(g_display_cache.line3, sizeof(g_display_cache.line3), "S:%.1f L:%.1f", data->soil_moisture, data->light_lux);
//            snprintf(g_display_cache.line4, sizeof(g_display_cache.line4), "P:%d L:%d B:%d", state->pump_on, state->light_on, state->beep_on);
//            break;

//        case PAGE_SENSOR:
//            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "SENSOR");
//            snprintf(g_display_cache.line2, sizeof(g_display_cache.line2), "TEMP: %.1f C", data->air_temp);
//            snprintf(g_display_cache.line3, sizeof(g_display_cache.line3), "HUMI: %.1f%%", data->air_humi);
//            snprintf(g_display_cache.line4, sizeof(g_display_cache.line4), "L:%.1f P:%.2f", data->light_lux, data->ph_value);
//            break;

//        case PAGE_PARAM:
//            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "PARAM");
//            snprintf(g_display_cache.line2, sizeof(g_display_cache.line2), "S_TH: %.1f", param->soil_low_th);
//            snprintf(g_display_cache.line3, sizeof(g_display_cache.line3), "L_TH: %.1f", param->light_low_th);
//            snprintf(g_display_cache.line4, sizeof(g_display_cache.line4), "PH:%.1f-%.1f", param->ph_low_th, param->ph_high_th);
//            break;

//        case PAGE_STATE:
//            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "STATE");
//            snprintf(g_display_cache.line2, sizeof(g_display_cache.line2), "P:%d L:%d B:%d", state->pump_on, state->light_on, state->beep_on);
//            snprintf(g_display_cache.line3, sizeof(g_display_cache.line3), "WIFI:%d", state->wifi_ok);
//            snprintf(g_display_cache.line4, sizeof(g_display_cache.line4), "CLOUD:%d", state->cloud_ok);
//            break;

//        default:
//            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "UNKNOWN");
//            break;
//    }

//    /* 真正刷新到 OLED */
//    BSP_OLED_Clear();
//    BSP_OLED_ShowString(0, 0, g_display_cache.line1);
//    BSP_OLED_ShowString(0, 2, g_display_cache.line2);
//    BSP_OLED_ShowString(0, 4, g_display_cache.line3);
//    BSP_OLED_ShowString(0, 6, g_display_cache.line4);
//    BSP_OLED_Refresh();
//}

///* APP_Display_GetCache（获取显示缓存） */
//const DisplayCache_t *APP_Display_GetCache(void)
//{
//    return &g_display_cache;
//}


#include "app_display.h"
#include "bsp_oled.h"
#include <stdio.h>
#include <string.h>

/* g_display_cache（显示缓存） */
static DisplayCache_t g_display_cache;

/* APP_Display_ClearCache（清空显示缓存） */
static void APP_Display_ClearCache(void)
{
    memset(&g_display_cache, 0, sizeof(g_display_cache));
}

/* APP_Display_Init（显示模块初始化） */
void APP_Display_Init(void)
{
    APP_Display_ClearCache();
}

/* APP_Display_Show（显示页面组织并刷新到 OLED） */
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
        case PAGE_SENSOR:
            /* 这一页固定显示全部关键传感器值
             * line1：温度 + 湿度
             * line2：土壤湿度原始值 + 光照
             * line3：pH
             * line4：输出状态
             */
            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1),
                     "T:%.1f H:%.1f", data->air_temp, data->air_humi);

            snprintf(g_display_cache.line2, sizeof(g_display_cache.line2),
                     "S:%.0f L:%.0f", data->soil_moisture, data->light_lux);

            snprintf(g_display_cache.line3, sizeof(g_display_cache.line3),
                     "P:%.2f", data->ph_value);

            snprintf(g_display_cache.line4, sizeof(g_display_cache.line4),
                     "W:%d L:%d B:%d", state->pump_on, state->light_on, state->beep_on);
            break;

        case PAGE_PARAM:
            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "PARAM");
            snprintf(g_display_cache.line2, sizeof(g_display_cache.line2), "S_TH: %.0f", param->soil_low_th);
            snprintf(g_display_cache.line3, sizeof(g_display_cache.line3), "L_TH: %.0f", param->light_low_th);
            snprintf(g_display_cache.line4, sizeof(g_display_cache.line4), "PH:%.1f-%.1f", param->ph_low_th, param->ph_high_th);
            break;

        case PAGE_STATE:
            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "STATE");
            snprintf(g_display_cache.line2, sizeof(g_display_cache.line2), "W:%d L:%d B:%d", state->pump_on, state->light_on, state->beep_on);
            snprintf(g_display_cache.line3, sizeof(g_display_cache.line3), "WIFI:%d", state->wifi_ok);
            snprintf(g_display_cache.line4, sizeof(g_display_cache.line4), "CLOUD:%d", state->cloud_ok);
            break;

        default:
            snprintf(g_display_cache.line1, sizeof(g_display_cache.line1), "UNKNOWN");
            break;
    }

    BSP_OLED_Clear();
    BSP_OLED_ShowString(0, 0, g_display_cache.line1);
    BSP_OLED_ShowString(0, 2, g_display_cache.line2);
    BSP_OLED_ShowString(0, 4, g_display_cache.line3);
    BSP_OLED_ShowString(0, 6, g_display_cache.line4);
    BSP_OLED_Refresh();
}

/* APP_Display_GetCache（获取显示缓存） */
const DisplayCache_t *APP_Display_GetCache(void)
{
    return &g_display_cache;
}

