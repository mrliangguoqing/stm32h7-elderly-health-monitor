/**
 * @file    app_voice_service.h
 * @brief   应用层语音交互逻辑模块头文件
 */

#ifndef __APP_VOICE_SERVICE_H
#define __APP_VOICE_SERVICE_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

extern SemaphoreHandle_t xVoiceDataReadySem;

void App_Voice_Service_Init(void);

#endif
