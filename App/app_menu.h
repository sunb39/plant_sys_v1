#ifndef __APP_MENU_H
#define __APP_MENU_H

#include <stdint.h>

/* PageId_t（页面编号枚举）
 * 作用：定义系统当前可切换的界面页面
 */
typedef enum
{
    PAGE_MAIN = 0,      /* 主页面 */
    PAGE_SENSOR,        /* 传感器页面 */
    PAGE_PARAM,         /* 参数页面 */
    PAGE_STATE,         /* 状态页面 */
    PAGE_MAX
} PageId_t;

void APP_Menu_Init(void);
PageId_t APP_Menu_GetPage(void);
void APP_Menu_NextPage(void);
void APP_Menu_PrevPage(void);

#endif
