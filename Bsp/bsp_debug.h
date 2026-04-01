#ifndef __BSP_DEBUG_H
#define __BSP_DEBUG_H

#include "main.h"

/* BSP_Debug_Init（调试串口初始化占位接口）
 * 说明：当前 USART1 已经由 CubeMX 初始化，
 * 这里先预留一个调试模块接口，后续扩展更方便。
 */
void BSP_Debug_Init(void);

/* BSP_Debug_Printf（调试打印接口）
 * 说明：后续可以统一从这里输出调试信息。
 */
void BSP_Debug_Printf(const char *fmt, ...);

#endif
