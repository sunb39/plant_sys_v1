#ifndef __BSP_BH1750_H
#define __BSP_BH1750_H

#include "main.h"

HAL_StatusTypeDef BSP_BH1750_Init(void);
HAL_StatusTypeDef BSP_BH1750_ReadLux(float *lux);

#endif
