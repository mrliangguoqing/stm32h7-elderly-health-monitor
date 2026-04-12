/**
 * @file    app_sensor.c
 * @brief   应用层传感器读取数据逻辑模块
 */

#include "app_sensor.h"

#include "FreeRTOS.h"
#include "task.h"

#include "bsp_aht30.h"
#include "bsp_bh1750.h"
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
    for (;;)
    {
        BSP_AHT30_UpdateData();  /* 更新 AHT30 温湿度数据 */
        BSP_BH1750_UpdateData(); /* BH1750 光照强度数据 */
        BSP_MQ5_UpdateData();    /* MQ-5 可燃气体数据 */

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief  传感器模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_Sensor_Init(void)
{

    /* BSP 层模块初始化 */
    BSP_AHT30_Init();
    BSP_BH1750_Init();
    BSP_MQ5_Init();

    xTaskCreate(Sensor_Task,
                "Sensor_Task",
                512,
                NULL,
                5,
                &xSensorTaskHandle);
}
