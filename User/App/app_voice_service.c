/**
 * @file    app_voice_service.c
 * @brief   应用层语音交互逻辑模块
 */

#include "app_voice_service.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "usart.h"

#include "app_alarm_notify.h"

#include "bsp_uart_comm.h"
#include "bsp_aht30.h"
#include "bsp_ds1302.h"
#include "bsp_esp8266.h"

#include "pal_log.h"

#include "utils_math.h"

#include <stdint.h>
#include <string.h>

/* 定义队列句柄 */
QueueHandle_t xVoiceQueue = NULL;

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xVoiceServiceTaskHandle = NULL;

static void Voice_ResponseFeedback(uart_control_t *p_voice_conn);
static void Voice_ActiveBroadcast(uart_control_t *p_voice_conn, Voice_Msg_Type_t type);

/**
 * @brief  语音交互处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Voice_Service_Task(void *pvParameters)
{
    Voice_Msg_Type_t received_msg;

    for (;;)
    {
        /* 阻塞等待队列消息 */
        if (xQueueReceive(xVoiceQueue, &received_msg, portMAX_DELAY) == pdPASS)
        {
            switch (received_msg)
            {
            case MSG_VOICE_UART_RX:
                Voice_ResponseFeedback(&voice_conn); /* 被动相应，根据指令发送相应数据 */
                BSP_UART_ClearBuffer(&voice_conn);   /* 重置串口接收缓冲区状态 */
                break;

            case MSG_VOICE_FALL_ALARM:
            case MSG_VOICE_HELP_ALARM:
                Voice_ActiveBroadcast(&voice_conn, received_msg); /* 系统主动触发播报 */
                break;

            default:
                break;
            }
        }
    }
}

/**
 * @brief  语音交互模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_Voice_Service_Init(void)
{
    /* 创建消息队列：深度10，单元大小为枚举类型 */
    if (xVoiceQueue == NULL)
    {
        xVoiceQueue = xQueueCreate(10, sizeof(Voice_Msg_Type_t));
    }

    xTaskCreate(Voice_Service_Task,
                "Voice",
                512,
                NULL,
                10,
                &xVoiceServiceTaskHandle);
}

/**
 * @brief  被动响应，根据收到的指令类型，打包并发送对应的传感器/系统数据
 * @param  p_voice_conn: 串口连接句柄
 */
static void Voice_ResponseFeedback(uart_control_t *p_voice_conn)
{
    if (p_voice_conn == NULL || p_voice_conn->rx_buf[0] != 0xAA)
    {
        return;
    }

    const aht30_data_t *p_aht30_data = BSP_AHT30_GetData();
    ds1302_data_t *p_ds1302_data = BSP_DS1302_GetData();
    const WeatherDay *daily_0 = &g_weather_info.daily[0];

    uint8_t tx_buffer[16] = {0};
    uint8_t tx_len = 0;
    int32_t integer_val = 0;
    uint32_t decimal_val = 0;

    /* 调试打印：转发原始数据到 Debug 串口 */
    HAL_UART_Transmit(&huart1, p_voice_conn->rx_buf, p_voice_conn->rx_cnt, 100);

    switch (p_voice_conn->rx_buf[1])
    {
    case 0x01: /* 求救指令 */
        if (xAlarmSemaphore != NULL) /* 释放信号量，唤醒报警任务 */
        {
            xSemaphoreGive(xAlarmSemaphore);
        }
        break;

    case 0x02: /* 天气预报 */
        tx_buffer[0] = 0xAA;
        tx_buffer[1] = 0x02;
        tx_buffer[2] = 0x01;
        tx_buffer[3] = daily_0->code_day;
        tx_buffer[4] = daily_0->high;
        tx_buffer[5] = daily_0->low;
        tx_buffer[6] = daily_0->precip;
        tx_buffer[7] = 0x55;
        tx_len = 8;
        break;

    case 0x03: /* 时间 */
        tx_buffer[0] = 0xAA;
        tx_buffer[1] = 0x03;
        tx_buffer[2] = 0x01;
        tx_buffer[3] = p_ds1302_data->hour;
        tx_buffer[4] = p_ds1302_data->minute;
        tx_buffer[5] = 0x55;
        tx_len = 6;
        break;

    case 0x04: /* 室内温湿度 */
        tx_buffer[0] = 0xAA;
        tx_buffer[1] = 0x04;
        tx_buffer[2] = 0x01;
        UTILS_Math_SplitFloat(p_aht30_data->temperature, 1, &integer_val, &decimal_val, NULL);
        tx_buffer[3] = (uint8_t)integer_val;
        tx_buffer[4] = (uint8_t)decimal_val;
        UTILS_Math_SplitFloat(p_aht30_data->humidity, 1, &integer_val, &decimal_val, NULL);
        tx_buffer[5] = (uint8_t)integer_val;
        tx_buffer[6] = (uint8_t)decimal_val;
        tx_buffer[7] = 0x55;
        tx_len = 8;
        break;
    }

    if (tx_len > 0)
    {
        HAL_UART_Transmit(p_voice_conn->huart, tx_buffer, tx_len, 100);
        HAL_UART_Transmit(&huart1, tx_buffer, tx_len, 100); /* Debug同步 */
    }
}

/**
 * @brief  主动播报，发送预设的紧急报警语音指令
 * @param  p_conn: 串口连接句柄
 * @param  type: 报警类型 (跌倒/求助)
 */
static void Voice_ActiveBroadcast(uart_control_t *p_voice_conn, Voice_Msg_Type_t type)
{
    if (p_voice_conn == NULL)
    {
        return;
    }

    const uint8_t fall_alarm_cmd[] = {0xAA, 0x01, 0x01, 0x55};
    const uint8_t help_alarm_cmd[] = {0xAA, 0x01, 0x02, 0x55};

    if (type == MSG_VOICE_FALL_ALARM)
    {
        HAL_UART_Transmit(p_voice_conn->huart, (uint8_t *)fall_alarm_cmd, 4, 100);
    }
    else if (type == MSG_VOICE_HELP_ALARM)
    {
        HAL_UART_Transmit(p_voice_conn->huart, (uint8_t *)help_alarm_cmd, 4, 100);
    }
}
