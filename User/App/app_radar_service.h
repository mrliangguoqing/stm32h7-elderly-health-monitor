/**
 * @file    app_radar_service.h
 * @brief   应用层雷达监测逻辑模块头文件
 */
#ifndef __APP_RADAR_SERVICE_H
#define __APP_RADAR_SERVICE_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

extern SemaphoreHandle_t xRadarDataReadySem;

void App_Radar_Service_Init(void);

#endif
