/**
 * @file    app_alarm_notify.h
 * @brief   应用层报警通知逻辑模块头文件
 */

#ifndef __APP_ALARM_NOTIFY_H
#define __APP_ALARM_NOTIFY_H

#include "FreeRTOS.h"
#include "semphr.h"

extern SemaphoreHandle_t xAlarmSemaphore;

void App_Alarm_Notify_Init(void);

#endif
