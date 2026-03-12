/**
 * @file    app_max30102.h
 * @brief   应用层 Max30102 采集与计算逻辑模块
 */
 
#ifndef __APP_MAX30102_H
#define __APP_MAX30102_H

#include "FreeRTOS.h"
#include "semphr.h"

extern SemaphoreHandle_t xMax30102DataReadySem;

void App_Max30102_Init(void);

#endif
