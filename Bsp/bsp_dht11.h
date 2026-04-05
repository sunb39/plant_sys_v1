#ifndef __BSP_DHT11_H
#define __BSP_DHT11_H

#include "main.h"

HAL_StatusTypeDef BSP_DHT11_Init(void);
HAL_StatusTypeDef BSP_DHT11_Read(float *temp, float *humi);

#endif



