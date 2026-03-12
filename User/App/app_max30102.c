/**
 * @file    app_max30102.c
 * @brief   应用层 Max30102 采集与计算逻辑模块
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "app_max30102.h"

#include "pal_log.h"

#include "bsp_max30102.h"
#include "bsp_max30102_algorithm.h"

#include <string.h>

/* 定义二值信号量，用于同步中断与任务 */
SemaphoreHandle_t xMax30102DataReadySem = NULL;

/* 任务句柄 */
TaskHandle_t xMax30102CollectTaskHandle = NULL;
TaskHandle_t xMax30102CalculateTaskHandle = NULL;

/**
 * @brief  MAX30102 采集数据任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
void MAX30102_Collect_Task(void *pvParameters)
{
    /* 初始数据采集 (首次填满 500 个点) */
    for (int i = 0; i < MAX30102_BUFFER_LEN; i++)
    {
        if (xSemaphoreTake(xMax30102DataReadySem, pdMS_TO_TICKS(20)))
        {
            BSP_MAX30102_ReadFifo(&max30102_handle.buffer.red_buffer[i],
                                  &max30102_handle.buffer.ir_buffer[i]);
        }
    }

    /* 首次采集完成后，通知计算任务进行第一次计算 */
    xTaskNotifyGive(xMax30102CalculateTaskHandle);

    for (;;)
    {
        /* 循环采集最新的 100 个点 */
        /* 注意：始终填充 buffer 的最后 100 个位置 (400~499) */
        for (int i = 400; i < 500; i++)
        {
            if (xSemaphoreTake(xMax30102DataReadySem, pdMS_TO_TICKS(20)) == pdTRUE)
            {
                BSP_MAX30102_ReadFifo(&max30102_handle.buffer.red_buffer[i],
                                      &max30102_handle.buffer.ir_buffer[i]);
            }
        }

        /* 采集够 100 个点了，唤醒计算任务 */
        if (xMax30102CalculateTaskHandle != NULL)
        {
            xTaskNotifyGive(xMax30102CalculateTaskHandle);
        }
    }
}

/**
 * @brief  MAX30102 计算任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
void MAX30102_Calculate_Task(void *pvParameters)
{
    for (;;)
    {
        /* 等待采集任务的通知 (无数据时进入阻塞态，不占 CPU) */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* 检查手指是否按下 (判断最新的采样点) */
        uint32_t current_ir_value = max30102_handle.buffer.ir_buffer[499];

        if (current_ir_value < 57000)
        {
            /* 手指松开，清空结果并跳过计算 */
            max30102_handle.data.heart_rate = 0;
            max30102_handle.data.spo2 = 0;
            max30102_handle.data.heart_rate_valid = 0;
            max30102_handle.data.spo2_valid = 0;
            PAL_LOG(PAL_LOG_LEVEL_INFO, "Finger removed. Waiting...");
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else
        {
            /* 手指按下，执行计算逻辑 */

            /* 执行算法 */
            maxim_heart_rate_and_oxygen_saturation(
                max30102_handle.buffer.ir_buffer, MAX30102_BUFFER_LEN,
                max30102_handle.buffer.red_buffer,
                &max30102_handle.data.spo2, &max30102_handle.data.spo2_valid,
                &max30102_handle.data.heart_rate, &max30102_handle.data.heart_rate_valid);

            /* 输出结果 */
            if (max30102_handle.data.heart_rate_valid || max30102_handle.data.spo2_valid)
            {
                PAL_LOG(PAL_LOG_LEVEL_DEBUG, "HR: %d bpm | SpO2: %d%%",
                        max30102_handle.data.heart_rate, max30102_handle.data.spo2);
            }
            else
            {
                PAL_LOG(PAL_LOG_LEVEL_INFO, "Searching for pulse...");
            }

            /* 为下一次计算准备缓冲区：滑动窗口 (丢弃旧的 100 个，保留后的 400 个) */
            /* 在计算完后再搬移，确保下次采集直接覆盖 400~499 */
            memcpy(&max30102_handle.buffer.red_buffer[0], &max30102_handle.buffer.red_buffer[100], 400 * sizeof(uint32_t));
            memcpy(&max30102_handle.buffer.ir_buffer[0], &max30102_handle.buffer.ir_buffer[100], 400 * sizeof(uint32_t));
        }
    }
}

/**
 * @brief 系统任务初始化
 */

/**
 * @brief  Max30102 模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_Max30102_Init(void)
{

    /* 创建信号量，用于外部中断回调函数中通知采集任务 */
    if (xMax30102DataReadySem == NULL)
    {
        xMax30102DataReadySem = xSemaphoreCreateBinary();
    }

    /* 采集任务：优先级设为高 (3) */
    xTaskCreate(MAX30102_Collect_Task,
                "M30102_Coll",
                256,
                NULL,
                3,
                &xMax30102CollectTaskHandle);

    /* 计算任务：优先级设为中 (2) */
    xTaskCreate(MAX30102_Calculate_Task,
                "M30102_Calc",
                256,
                NULL,
                2,
                &xMax30102CalculateTaskHandle);
}
