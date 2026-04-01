#ifndef __APP_DISPLAY_H
#define __APP_DISPLAY_H

#include "plant_types.h"
#include "app_menu.h"

/* DisplayCache_t（显示缓存结构体）
 * 作用：保存当前页面准备显示的数据，
 * 便于后续移植到 OLED 时直接显示。
 */
typedef struct
{
    char line1[32];
    char line2[32];
    char line3[32];
    char line4[32];
} DisplayCache_t;

void APP_Display_Init(void);
void APP_Display_Show(PageId_t page,
                      const SensorData_t *data,
                      const SystemParam_t *param,
                      const SystemState_t *state);

const DisplayCache_t *APP_Display_GetCache(void);

#endif
