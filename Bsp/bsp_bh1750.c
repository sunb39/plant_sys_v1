#include "bsp_bh1750.h"
#include "i2c.h"

#define BH1750_ADDR         (0x23 << 1)   // ADDR接GND时，7位地址0x23，HAL里左移1位
#define BH1750_POWER_ON     0x01
#define BH1750_RESET        0x07
#define BH1750_CONT_H_RES   0x10          // 连续高分辨率模式

HAL_StatusTypeDef BSP_BH1750_Init(void)
{
    uint8_t cmd;

    cmd = BH1750_POWER_ON;
    if (HAL_I2C_Master_Transmit(&hi2c1, BH1750_ADDR, &cmd, 1, 100) != HAL_OK)
    {
        return HAL_ERROR;
    }

    cmd = BH1750_RESET;
    if (HAL_I2C_Master_Transmit(&hi2c1, BH1750_ADDR, &cmd, 1, 100) != HAL_OK)
    {
        return HAL_ERROR;
    }

    cmd = BH1750_CONT_H_RES;
    if (HAL_I2C_Master_Transmit(&hi2c1, BH1750_ADDR, &cmd, 1, 100) != HAL_OK)
    {
        return HAL_ERROR;
    }

    HAL_Delay(180);   // 等待首次测量完成
    return HAL_OK;
}

HAL_StatusTypeDef BSP_BH1750_ReadLux(float *lux)
{
    uint8_t buf[2];
    uint16_t raw;

    if (lux == 0)
    {
        return HAL_ERROR;
    }

    if (HAL_I2C_Master_Receive(&hi2c1, BH1750_ADDR, buf, 2, 100) != HAL_OK)
    {
        return HAL_ERROR;
    }

    raw = ((uint16_t)buf[0] << 8) | buf[1];
    *lux = raw / 1.2f;

    return HAL_OK;
}
