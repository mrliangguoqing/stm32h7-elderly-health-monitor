#ifndef __BSP_AHT30_H
#define __BSP_AHT30_H

#include "aht30.h"
#include "i2c.h"

/* 函数声明 */
uint8_t BSP_AHT30_Init(void);
uint8_t BSP_AHT30_Update(void);
AHT30_Data_t BSP_AHT30_GetData(void);

#endif
