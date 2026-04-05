#ifndef __BSP_SOIL_H
#define __BSP_SOIL_H

#include "main.h"

HAL_StatusTypeDef BSP_Soil_Init(void);
HAL_StatusTypeDef BSP_Soil_ReadRaw(uint16_t *raw);

#endif


