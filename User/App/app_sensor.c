/**
 * @file    app_sensor.c
 * @brief   应用层传感器读取数据逻辑模块
 */

#include "app_sensor.h"

#include "FreeRTOS.h"
#include "task.h"

#include "bsp_mq5.h"

#include "pal_log.h"

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xSensorTaskHandle = NULL;

/**
 * @brief  传感器处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Sensor_Task(void *pvParameters)
{
    const mq5_data_t *px_mq5_data = NULL;

    /* 获取指针 */
    px_mq5_data = BSP_MQ5_GetData();

    for (;;)
    {

        /* 处理 MQ-5 传感器 */
        if (px_mq5_data != NULL)
        {
            BSP_MQ5_Process();
            PAL_LOG(PAL_LOG_LEVEL_DEBUG, "Voltage: %.2f V\n", px_mq5_data->smooth_v);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief  传感器模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_Sensor_Init(void)
{
    xTaskCreate(Sensor_Task,
                "Sensor_Task",
                256,
                NULL,
                1,
                &xSensorTaskHandle);
}
