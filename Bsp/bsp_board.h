#ifndef __BSP_BOARD_H
#define __BSP_BOARD_H

#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "usart.h"

/* =========================================================
 *  OLED 硬件配置
 * =========================================================
 */

/* OLED 使用 I2C1（CubeMX 中已经配置为 PB6/PB7） */
#define OLED_I2C_HANDLE         hi2c1

/* 常见 0.96 寸 I2C OLED 地址：
 * 7位地址常见是 0x3C，对 HAL_I2C_Master_Transmit 一般传 8位地址 0x78
 * 如果后面上电不显示，可改成 0x7A 再试
 */
#define OLED_I2C_ADDR           0x78

#define OLED_WIDTH              128
#define OLED_HEIGHT             64
#define OLED_PAGE_NUM           (OLED_HEIGHT / 8)

/* =========================================================
 *  串口调试 / ESP8266
 * =========================================================
 */

/* 调试串口和 ESP8266 都先走 USART1 */
#define DEBUG_UART_HANDLE       huart1

/* =========================================================
 *  执行器输出引脚定义
 * =========================================================
 */

/* 水泵控制引脚：PB0 */
#define PUMP_GPIO_PORT          GPIOB
#define PUMP_GPIO_PIN           GPIO_PIN_0

/* 补光 LED 控制引脚：PB1 */
#define LIGHT_GPIO_PORT         GPIOB
#define LIGHT_GPIO_PIN          GPIO_PIN_1

/* 蜂鸣器控制引脚：PB13 */
#define BEEP_GPIO_PORT          GPIOB
#define BEEP_GPIO_PIN           GPIO_PIN_13

/* =========================================================
 *  有效电平定义
 * =========================================================
 *  这样做的好处：
 *  以后如果某个模块是低电平触发，只需要改这里，不用改业务代码
 */

/* 你的 MOSFET 水泵驱动资料写的是“高电平启动” */
#define PUMP_ACTIVE_LEVEL       GPIO_PIN_SET
#define PUMP_INACTIVE_LEVEL     GPIO_PIN_RESET

/* 补光默认按高电平有效处理 */
#define LIGHT_ACTIVE_LEVEL      GPIO_PIN_SET
#define LIGHT_INACTIVE_LEVEL    GPIO_PIN_RESET

/* 蜂鸣器先按高电平有效处理
 * 如果后面你接的是低电平触发蜂鸣器模块，就把这两行改成相反即可
 */
#define BEEP_ACTIVE_LEVEL       GPIO_PIN_SET
#define BEEP_INACTIVE_LEVEL     GPIO_PIN_RESET

#endif
