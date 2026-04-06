//#ifndef __BSP_SOIL_H
//#define __BSP_SOIL_H

//#include "main.h"

//HAL_StatusTypeDef BSP_Soil_Init(void);
//HAL_StatusTypeDef BSP_Soil_ReadRaw(uint16_t *raw);

//#endif

#ifndef __BSP_SOIL_H
#define __BSP_SOIL_H

#include "main.h"

/* BSP_Soil_Init（土壤湿度 / pH ADC 初始化） */
HAL_StatusTypeDef BSP_Soil_Init(void);

/* BSP_Soil_ReadRaw（读取土壤湿度原始 ADC） */
HAL_StatusTypeDef BSP_Soil_ReadRaw(uint16_t *raw);

/* BSP_PH_ReadRaw（读取 pH 原始 ADC） */
HAL_StatusTypeDef BSP_PH_ReadRaw(uint16_t *raw);

#endif
