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

#include <stdint.h>
#include <string.h>

/* 定义二值信号量，用于同步中断与任务 */
SemaphoreHandle_t xVoiceDataReadySem = NULL;

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xVoiceServiceTaskHandle = NULL;

static void split_float(float f_val, uint8_t *int_part, uint8_t *dec_part);

/**
 * @brief  语音交互处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Voice_Service_Task(void *pvParameters)
{
    const aht30_data_t *p_aht30_data = BSP_AHT30_GetData();
    ds1302_data_t *p_ds1302_data = BSP_DS1302_GetData();

    uint8_t tx_buffer[16] = {0};
    uint8_t integer_val = 0, decimal_val = 0;

    for (;;)
    {
        /* 阻塞等待信号量 */
        if (xSemaphoreTake(xVoiceDataReadySem, portMAX_DELAY) == pdPASS)
        {
            if (voice_conn.rx_buf[0] == 0xAA)
            {
                HAL_UART_Transmit(&huart1, voice_conn.rx_buf, voice_conn.rx_cnt, 100);

                switch (voice_conn.rx_buf[1])
                {
                case 0x01: /* 求救指令 */
                    /* 释放信号量，唤醒报警任务 */
                    if (xAlarmSemaphore != NULL)
                    {
                        xSemaphoreGive(xAlarmSemaphore);
                    }
                    break;
                case 0x02: /* 天气预报 */
                    // tx_buffer[0] = 0xAA;
                    // tx_buffer[1] = 0x02;
                    // tx_buffer[2] = 0x01;
                    // tx_buffer[3] = ;
                    // tx_buffer[4] = ;
                    // tx_buffer[5] = ;
                    // tx_buffer[6] = ;
                    // tx_buffer[7] = 0x55;
                    // HAL_UART_Transmit(&huart1, tx_buffer, 8, 100);
                    // HAL_UART_Transmit(voice_conn.huart, tx_buffer, 8, 100);

                    break;

                case 0x03: /* 时间 */
                    tx_buffer[0] = 0xAA;
                    tx_buffer[1] = 0x03;
                    tx_buffer[2] = 0x01;
                    tx_buffer[3] = p_ds1302_data->hour;
                    tx_buffer[4] = p_ds1302_data->minute;
                    tx_buffer[5] = 0x55;
                    HAL_UART_Transmit(&huart1, tx_buffer, 6, 100);
                    HAL_UART_Transmit(voice_conn.huart, tx_buffer, 6, 100);

                    break;
                case 0x04: /* 室内温湿度 */
                    tx_buffer[0] = 0xAA;
                    tx_buffer[1] = 0x04;
                    tx_buffer[2] = 0x01;
                    split_float(p_aht30_data->temperature, &integer_val, &decimal_val);
                    tx_buffer[3] = integer_val;
                    tx_buffer[4] = decimal_val;
                    split_float(p_aht30_data->humidity, &integer_val, &decimal_val);
                    tx_buffer[5] = integer_val;
                    tx_buffer[6] = decimal_val;
                    tx_buffer[7] = 0x55;
                    // tx_buffer[8] = '\0';
                    HAL_UART_Transmit(&huart1, tx_buffer, 9, 100);
                    HAL_UART_Transmit(voice_conn.huart, tx_buffer, 9, 100);
                    break;
                }
            }

            BSP_UART_ClearBuffer(&voice_conn);
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
    /* 创建信号量，用于外部中断回调函数中通知采集任务 */
    if (xVoiceDataReadySem == NULL)
    {
        xVoiceDataReadySem = xSemaphoreCreateBinary();
    }

    xTaskCreate(Voice_Service_Task,
                "Voice",
                256,
                NULL,
                10,
                &xVoiceServiceTaskHandle);
}

/**
 * @brief  将浮点数拆分为整数部分和小数部分（保留2位小数）
 * @param  f_val    [in]  输入的浮点数 (建议范围 0.00 至 255.99)
 * @param  int_part [out] 存储整数部分的指针
 * @param  dec_part [out] 存储两位小数部分的指针 (取值范围 00-99)
 * @return None
 */
static void split_float(float f_val, uint8_t *int_part, uint8_t *dec_part)
{
    /* 处理负数（如果是负数，先取绝对值） */
    if (f_val < 0)
    {
        f_val = -f_val;
    }

    *int_part = (uint8_t)f_val; /* 提取整数部分 */

    *dec_part = (uint8_t)((f_val - (float)(*int_part)) * 100.0f + 0.5f); /* 提取小数部分，保留 2 位并四舍五入 */

    /* 特殊边界处理，如果小数进位到了 100（例如 0.996 变 100） */
    if (*dec_part >= 100)
    {
        *dec_part = 0;
        (*int_part)++;
    }
}
