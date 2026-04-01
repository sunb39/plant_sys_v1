#include "app_menu.h"

/* g_current_page（当前页面变量） */
static PageId_t g_current_page = PAGE_MAIN;

/* APP_Menu_Init（菜单初始化） */
void APP_Menu_Init(void)
{
    g_current_page = PAGE_MAIN;
}

/* APP_Menu_GetPage（获取当前页面） */
PageId_t APP_Menu_GetPage(void)
{
    return g_current_page;
}

/* APP_Menu_NextPage（切换到下一页） */
void APP_Menu_NextPage(void)
{
    g_current_page = (PageId_t)((g_current_page + 1U) % PAGE_MAX);
}

/* APP_Menu_PrevPage（切换到上一页） */
void APP_Menu_PrevPage(void)
{
    if (g_current_page == PAGE_MAIN)
    {
        g_current_page = (PageId_t)(PAGE_MAX - 1U);
    }
    else
    {
        g_current_page = (PageId_t)(g_current_page - 1U);
    }
}
