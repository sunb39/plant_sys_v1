#ifndef __APP_DISPLAY_H
#define __APP_DISPLAY_H

#include "plant_types.h"
#include "app_menu.h"

/* APP_Display_Init（显示模块初始化）
 * 作用：初始化显示层变量
 */
void APP_Display_Init(void);

/* APP_Display_Show（显示界面刷新）
 * 作用：根据当前页面组织需要显示的数据
 */
void APP_Display_Show(PageId_t page,
                      const SensorData_t *data,
                      const SystemParam_t *param,
                      const SystemState_t *state);

#endif
