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

#include <string.h>

/* 定义二值信号量，用于同步中断与任务 */
SemaphoreHandle_t xMax30102DataReadySem = NULL;

/* 任务句柄 */
TaskHandle_t xMax30102CollectTaskHandle = NULL;
TaskHandle_t xMax30102CalculateTaskHandle = NULL;

void MAX30102_Update_Buffer(uint16_t start_idx, uint16_t count);

/**
 * @brief  MAX30102 采集数据任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
void MAX30102_Collect_Task(void *pvParameters)
{
    MAX30102_Update_Buffer(0, MAX30102_BUFFER_LEN); /* 初始采集：填满整个 500 点的缓冲区 */
    xTaskNotifyGive(xMax30102CalculateTaskHandle);  /* 首次采集完成后，通知计算任务进行第一次计算 */

    for (;;)
    {
        /* 循环采集，始终只填充 buffer 的最后 100 个位置 (400~499) */
        /* 注意：这里不需要做内存滑动，只管填坑 */
        MAX30102_Update_Buffer(400, 100);

        /* 采集够 100 个点，唤醒计算任务 */
        xTaskNotifyGive(xMax30102CalculateTaskHandle);
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

        if (current_ir_value < MAX30102_TOUCH_THRESHOLD)
        {
            /* 状态：手指未按下，清空结果并跳过计算 */
            BSP_MAX30102_ResetResults();
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else
        {
            /* 状态：手指已按下，正在计算或已完成 */
            BSP_MAX30102_ExecuteAlgorithm();

            // /* 输出结果 */
            // if (max30102_handle.data.heart_rate_valid || max30102_handle.data.spo2_valid)
            // {
            //     PAL_LOG(PAL_LOG_LEVEL_DEBUG, "HR_S: %3d | HR_R: %3d | SpO2: %2d%% | [Stat] HR_V:%d SpO2_V:%d",
            //             max30102_handle.data.stable_heart_rate,
            //             max30102_handle.data.heart_rate,
            //             max30102_handle.data.spo2,
            //             max30102_handle.data.heart_rate_valid,
            //             max30102_handle.data.spo2_valid);
            // }
            // else
            // {
            //     PAL_LOG(PAL_LOG_LEVEL_INFO, "Searching for pulse...");
            // }

            /* 为下一次计算准备缓冲区：滑动窗口 (丢弃旧的 100 个，保留后的 400 个) */
            /* 在计算完后再搬移，确保下次采集直接覆盖 400~499 */
            memcpy(&max30102_handle.buffer.red_buffer[0], &max30102_handle.buffer.red_buffer[100], 400 * sizeof(uint32_t));
            memcpy(&max30102_handle.buffer.ir_buffer[0], &max30102_handle.buffer.ir_buffer[100], 400 * sizeof(uint32_t));
        }
    }
}

/**
 * @brief  Max30102 模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_Max30102_Init(void)
{
    /* BSP 层模块初始化 */
    BSP_MAX30102_Init();

    /* 创建信号量，用于外部中断回调函数中通知采集任务 */
    if (xMax30102DataReadySem == NULL)
    {
        xMax30102DataReadySem = xSemaphoreCreateBinary();
    }

    /* 采集任务 */
    xTaskCreate(MAX30102_Collect_Task,
                "M30102_Coll",
                512,
                NULL,
                15,
                &xMax30102CollectTaskHandle);

    /* 算法任务 */
    xTaskCreate(MAX30102_Calculate_Task,
                "M30102_Calc",
                512,
                NULL,
                7,
                &xMax30102CalculateTaskHandle);
}

/**
 * @brief  更新 MAX30102 缓冲区数据
 * @param  start_idx: 开始填充的索引位置
 * @param  count: 需要采集的数据点数
 * @retval None
 */
void MAX30102_Update_Buffer(uint16_t start_idx, uint16_t count)
{
    for (int i = 0; i < count; i++)
    {
        /* 阻塞等待传感器数据准备就绪的信号量 */
        if (xSemaphoreTake(xMax30102DataReadySem, pdMS_TO_TICKS(20)) == pdTRUE)
        {
            uint16_t target_idx = start_idx + i;
            if (target_idx < MAX30102_BUFFER_LEN)
            {
                BSP_MAX30102_ReadFifo(&max30102_handle.buffer.red_buffer[target_idx],
                                      &max30102_handle.buffer.ir_buffer[target_idx]);
            }
        }
    }
}
