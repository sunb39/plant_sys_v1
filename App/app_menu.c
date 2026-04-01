#include "app_menu.h"

/* g_menu（全局菜单上下文）
 * 作用：保存当前菜单的状态
 */
static MenuContext_t g_menu;

/* APP_Menu_Init（菜单初始化）
 * 说明：系统上电后默认进入主页面，选中项归零，处于浏览模式。
 */
void APP_Menu_Init(void)
{
    g_menu.current_page = PAGE_MAIN;
    g_menu.current_item = 0U;
    g_menu.edit_mode    = MENU_VIEW;
}

/* APP_Menu_GetPage（获取当前页面） */
PageId_t APP_Menu_GetPage(void)
{
    return g_menu.current_page;
}

/* APP_Menu_GetItem（获取当前选中项） */
uint8_t APP_Menu_GetItem(void)
{
    return g_menu.current_item;
}

/* APP_Menu_GetEditMode（获取当前编辑模式） */
MenuEdit_t APP_Menu_GetEditMode(void)
{
    return g_menu.edit_mode;
}

/* APP_Menu_NextPage（切换到下一页）
 * 说明：切换页面时默认回到浏览模式，并将选中项归零。
 */
void APP_Menu_NextPage(void)
{
    g_menu.current_page = (PageId_t)((g_menu.current_page + 1U) % PAGE_MAX);
    g_menu.current_item = 0U;
    g_menu.edit_mode    = MENU_VIEW;
}

/* APP_Menu_PrevPage（切换到上一页） */
void APP_Menu_PrevPage(void)
{
    if (g_menu.current_page == PAGE_MAIN)
    {
        g_menu.current_page = (PageId_t)(PAGE_MAX - 1U);
    }
    else
    {
        g_menu.current_page = (PageId_t)(g_menu.current_page - 1U);
    }

    g_menu.current_item = 0U;
    g_menu.edit_mode    = MENU_VIEW;
}

/* APP_Menu_NextItem（切换到下一项）
 * 说明：当前先简单递增，后续可根据不同页面限制最大项数。
 */
void APP_Menu_NextItem(void)
{
    g_menu.current_item++;
}

/* APP_Menu_PrevItem（切换到上一项） */
void APP_Menu_PrevItem(void)
{
    if (g_menu.current_item > 0U)
    {
        g_menu.current_item--;
    }
}

/* APP_Menu_EnterEdit（进入编辑模式） */
void APP_Menu_EnterEdit(void)
{
    g_menu.edit_mode = MENU_EDIT;
}

/* APP_Menu_ExitEdit（退出编辑模式） */
void APP_Menu_ExitEdit(void)
{
    g_menu.edit_mode = MENU_VIEW;
}
