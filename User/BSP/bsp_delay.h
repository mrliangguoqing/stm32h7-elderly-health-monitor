/**
 * @file    bsp_delay.c
 * @brief   系统延时模块
 */

#ifndef __BSP_DELAY_H
#define __BSP_DELAY_H

#include "bsp_dwt.h"

#include <stdint.h>

void BSP_Delay_Init(void);
void BSP_DelayMs(uint32_t ms);

#define BSP_DelayUs(us)  BSP_DWT_DelayUs(us)

#endif
