/**
 * @file    app_voice_service.h
 * @brief   应用层语音交互逻辑模块头文件
 */

#ifndef __APP_VOICE_SERVICE_H
#define __APP_VOICE_SERVICE_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* 定义消息类型 */
typedef enum
{
    MSG_VOICE_UART_RX,    /* 语音助手串口收到回传数据 */
    MSG_VOICE_FALL_ALARM, /* 雷达检测到跌倒，请求播报 */
    MSG_VOICE_HELP_ALARM  /* 手动求救，请求播报 */
} Voice_Msg_Type_t;

void App_Voice_Service_Init(void);

/* 声明队列句柄 */
extern QueueHandle_t xVoiceQueue;

#endif
