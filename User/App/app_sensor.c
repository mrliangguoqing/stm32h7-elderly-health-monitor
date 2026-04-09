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
    const aht30_data_t *p_aht30_data = BSP_AHT30_GetData();
    const bh1750_data_t *p_bh1750_data = BSP_BH1750_GetData();
    const mq5_data_t *p_mq5_data = BSP_MQ5_GetData();

    for (;;)
    {
        /* 处理 AHT30 温湿度传感器 */
        if (p_aht30_data != NULL)
        {
            if (BSP_AHT30_UpdateData() == 0)
            {
                if (p_aht30_data != NULL)
                {
                    PAL_LOG(PAL_LOG_LEVEL_DEBUG, "AHT30-Temperature: %.2f °C", p_aht30_data->temperature);
                    PAL_LOG(PAL_LOG_LEVEL_DEBUG, "AHT30-Humidity   : %.2f %%", p_aht30_data->humidity);
                }
            }
        }

        /* 处理 BH1750 光照强度传感器 */
        if (p_bh1750_data != NULL)
        {
            if (BSP_BH1750_UpdateData() == 0)
            {
                PAL_LOG(PAL_LOG_LEVEL_DEBUG, "BH1750-Lux : %f", p_bh1750_data->lux);
            }
        }

        /* 处理 MQ-5 可燃气体传感器 */
        if (p_mq5_data != NULL)
        {
            if (BSP_MQ5_UpdateData() == 0)
            {
                PAL_LOG(PAL_LOG_LEVEL_DEBUG, "MQ5-Voltage: %.2f V", p_mq5_data->smooth_v);
            }
        }

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
