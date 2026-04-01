#ifndef __APP_MENU_H
#define __APP_MENU_H

#include <stdint.h>

/* PageId_t（页面编号枚举）
 * 作用：定义系统可切换的界面页面
 * 答辩可说：本系统采用多页面菜单结构，
 * 便于本地显示不同类型的信息，如实时数据、参数设置和系统状态。
 */
typedef enum
{
    PAGE_MAIN = 0,      /* 主页面：显示系统概览 */
    PAGE_SENSOR,        /* 传感器页面：显示环境参数 */
    PAGE_PARAM,         /* 参数页面：显示阈值参数 */
    PAGE_STATE,         /* 状态页面：显示设备运行状态 */
    PAGE_MAX
} PageId_t;

/* MenuEdit_t（菜单编辑状态枚举）
 * 作用：表示当前是否处于参数编辑模式
 */
typedef enum
{
    MENU_VIEW = 0,      /* 浏览模式 */
    MENU_EDIT           /* 编辑模式 */
} MenuEdit_t;

/* MenuContext_t（菜单上下文结构体）
 * 作用：统一保存当前页面、当前选中项和编辑状态
 */
typedef struct
{
    PageId_t  current_page;   /* current_page（当前页面） */
    uint8_t   current_item;   /* current_item（当前选中项） */
    MenuEdit_t edit_mode;     /* edit_mode（编辑模式） */
} MenuContext_t;

void APP_Menu_Init(void);
PageId_t APP_Menu_GetPage(void);
uint8_t APP_Menu_GetItem(void);
MenuEdit_t APP_Menu_GetEditMode(void);

void APP_Menu_NextPage(void);
void APP_Menu_PrevPage(void);

void APP_Menu_NextItem(void);
void APP_Menu_PrevItem(void);

void APP_Menu_EnterEdit(void);
void APP_Menu_ExitEdit(void);

#endif
