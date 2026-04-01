#include "bsp_debug.h"
#include "usart.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* 如果你使用的是 Keil MDK，下面这部分用于关闭半主机模式
 * 否则 printf 可能会报错，或者需要调试器支持。
 */
#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
};

FILE __stdout;
FILE __stdin;

/* _sys_exit（系统退出函数）
 * 说明：关闭半主机模式后需要提供该空函数。
 */
void _sys_exit(int x)
{
    (void)x;
}

/* fputc（字符输出重定向）
 * 说明：把 printf 的底层输出重定向到 USART1。
 * 每输出一个字符，就通过串口1发送出去。
 */
int fputc(int ch, FILE *f)
{
    uint8_t c = (uint8_t)ch;
    HAL_UART_Transmit(&huart1, &c, 1, 100);
    return ch;
}

/* BSP_Debug_Init（调试初始化）
 * 说明：当前串口已由 CubeMX 初始化，这里先留空即可。
 */
void BSP_Debug_Init(void)
{
    /* 以后如果需要初始化日志缓存、打印欢迎信息，可放在这里 */
}

/* BSP_Debug_Printf（带缓冲的调试打印）
 * 说明：相比直接 printf，这个接口更方便统一管理日志输出。
 */
void BSP_Debug_Printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
}
