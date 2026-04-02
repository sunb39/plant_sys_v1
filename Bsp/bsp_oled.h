#ifndef __BSP_OLED_H
#define __BSP_OLED_H

#include "bsp_board.h"
#include <stdint.h>

/* BSP_OLED_Init（OLED 初始化）
 * 作用：向 OLED 发送初始化命令，使其进入正常显示状态
 */
void BSP_OLED_Init(void);

/* BSP_OLED_Clear（清空显存缓冲区）
 * 作用：将软件缓冲区全部清零，但不会立刻更新到屏幕
 */
void BSP_OLED_Clear(void);

/* BSP_OLED_Refresh（刷新显示）
 * 作用：把当前缓冲区内容真正发送到 OLED 屏幕
 */
void BSP_OLED_Refresh(void);

/* BSP_OLED_Fill（整屏填充）
 * 作用：用指定字节填充整个缓冲区
 * 参数 fill_data：0x00 为全灭，0xFF 为全亮
 */
void BSP_OLED_Fill(uint8_t fill_data);

/* BSP_OLED_ShowChar（显示单个字符）
 * x：列坐标（0~127）
 * page：页坐标（0~7），每页高 8 像素
 * ch：要显示的字符
 */
void BSP_OLED_ShowChar(uint8_t x, uint8_t page, char ch);

/* BSP_OLED_ShowString（显示字符串）
 * x：列坐标（0~127）
 * page：页坐标（0~7）
 * str：字符串
 */
void BSP_OLED_ShowString(uint8_t x, uint8_t page, const char *str);

#endif
