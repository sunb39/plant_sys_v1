#include "bsp_dht11.h"
#include "tim.h"

#define DHT11_PORT GPIOB
#define DHT11_PIN  GPIO_PIN_12

static void DHT11_DelayUs(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

static void DHT11_Pin_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

static void DHT11_Pin_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

static HAL_StatusTypeDef DHT11_WaitForLevel(GPIO_PinState level, uint16_t timeout_us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) != level)
    {
        if (__HAL_TIM_GET_COUNTER(&htim2) >= timeout_us)
        {
            return HAL_TIMEOUT;
        }
    }
    return HAL_OK;
}

static HAL_StatusTypeDef DHT11_WaitWhileLevel(GPIO_PinState level, uint16_t timeout_us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == level)
    {
        if (__HAL_TIM_GET_COUNTER(&htim2) >= timeout_us)
        {
            return HAL_TIMEOUT;
        }
    }
    return HAL_OK;
}

static HAL_StatusTypeDef DHT11_ReadByte(uint8_t *byte)
{
    uint8_t i = 0;
    uint8_t value = 0;

    if (byte == 0)
    {
        return HAL_ERROR;
    }

    for (i = 0; i < 8; i++)
    {
        /* 等待数据位开始的高电平 */
        if (DHT11_WaitForLevel(GPIO_PIN_SET, 100) != HAL_OK)
        {
            return HAL_TIMEOUT;
        }

        /* 高电平持续 26~28us 表示 0，约 70us 表示 1
           在 40us 处采样 */
        DHT11_DelayUs(35);

        value <<= 1;
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
        {
            value |= 0x01;
        }

        /* 等待当前位结束，回到低电平 */
        if (DHT11_WaitWhileLevel(GPIO_PIN_SET, 100) != HAL_OK)
        {
            return HAL_TIMEOUT;
        }
    }

    *byte = value;
    return HAL_OK;
}

HAL_StatusTypeDef BSP_DHT11_Init(void)
{
    HAL_TIM_Base_Start(&htim2);

    DHT11_Pin_Output();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    HAL_Delay(1);

    return HAL_OK;
}

HAL_StatusTypeDef BSP_DHT11_Read(float *temp, float *humi)
{
    uint8_t buf[5] = {0};

    if ((temp == 0) || (humi == 0))
    {
        return HAL_ERROR;
    }

    /* 主机发送开始信号：拉低至少 18ms */
    DHT11_Pin_Output();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);

    /* 拉高 20~40us，然后切换输入 */
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    DHT11_DelayUs(30);
    DHT11_Pin_Input();

    /* 等待 DHT11 响应：低-高-低 */
    if (DHT11_WaitForLevel(GPIO_PIN_RESET, 100) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    if (DHT11_WaitForLevel(GPIO_PIN_SET, 100) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    if (DHT11_WaitForLevel(GPIO_PIN_RESET, 100) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    if (DHT11_WaitWhileLevel(GPIO_PIN_RESET, 100) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    /* 读取 5 字节 */
    if (DHT11_ReadByte(&buf[0]) != HAL_OK) return HAL_ERROR;
    if (DHT11_ReadByte(&buf[1]) != HAL_OK) return HAL_ERROR;
    if (DHT11_ReadByte(&buf[2]) != HAL_OK) return HAL_ERROR;
    if (DHT11_ReadByte(&buf[3]) != HAL_OK) return HAL_ERROR;
    if (DHT11_ReadByte(&buf[4]) != HAL_OK) return HAL_ERROR;

    /* 校验 */
    if (((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3])) != buf[4])
    {
        return HAL_ERROR;
    }

    *humi = (float)buf[0] + ((float)buf[1] * 0.1f);
    *temp = (float)buf[2] + ((float)buf[3] * 0.1f);

    return HAL_OK;
}
