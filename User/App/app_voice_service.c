/**
 * @file    app_voice_service.c
 * @brief   应用层语音交互逻辑模块
 */

#include "app_voice_service.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "usart.h"

#include "bsp_uart_comm.h"

#include "pal_log.h"

/* 定义二值信号量，用于同步中断与任务 */
SemaphoreHandle_t xVoiceDataReadySem = NULL;

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xVoiceServiceTaskHandle = NULL;

/**
 * @brief  语音交互处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Voice_Service_Task(void *pvParameters)
{
    for (;;)
    {
        /* 阻塞等待信号量 */
        if (xSemaphoreTake(xVoiceDataReadySem, portMAX_DELAY) == pdPASS)
        {
            if (voice_conn.rx_buf[0] == 0xAA)
            {
                HAL_UART_Transmit(&huart1, voice_conn.rx_buf, voice_conn.rx_cnt, 100);
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
