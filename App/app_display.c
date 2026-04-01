#include "app_display.h"

/* APP_Display_Init（显示模块初始化）
 * 当前阶段先保留空实现，后续接入 OLED 真正显示函数。
 */
void APP_Display_Init(void)
{
    /* 以后可在这里调用 OLED_Init() 等显示驱动函数 */
}

/* APP_Display_Show（显示界面刷新）
 * 当前阶段先完成页面逻辑分支，
 * 后续硬件到位后只需要在各分支里补上 OLED 显示代码。
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

    switch (page)
    {
        case PAGE_MAIN:
            /* 主页面：以后显示系统总览 */
            break;

        case PAGE_SENSOR:
            /* 传感器页面：以后显示温湿度、光照、土壤湿度、pH */
            break;

        case PAGE_PARAM:
            /* 参数页面：以后显示阈值和自动控制开关 */
            break;

        case PAGE_STATE:
            /* 状态页面：以后显示水泵、补光、报警、WiFi、云端状态 */
            break;

        default:
            break;
    }
}
