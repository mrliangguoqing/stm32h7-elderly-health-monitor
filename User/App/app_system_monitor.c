/**
 * @file    app_system_monitor.c
 * @brief   应用层系统监视模块
 * @details 负责监控 FreeRTOS 任务的情况，通过串口打印信息
 */

#include "app_system_monitor.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_aht30.h"

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xSystemMonitorTaskHandle = NULL;

/**
 * @brief  监控任务情况打印信息处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
void System_Monitor_Task(void *pvParameters)
{
    for (;;)
    {
        if (BSP_AHT30_Update() == 0)
        {
            AHT30_Data_t current_data = BSP_AHT30_GetData();
            printf("AHT30 Sensor Data:\r\n");
            printf("Temperature: %.2f °C\r\n", current_data.temperature);
            printf("Humidity   : %.2f %%\r\n", current_data.humidity);
            printf("Status Byte: 0x%02X\r\n", current_data.status);
            printf("\r\n---------------------------\r\n\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/**
 * @brief  系统监视模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_System_Monitor_Init(void)
{
    xTaskCreate(System_Monitor_Task,
                "System_Monitor_Task",
                256,
                NULL,
                1,
                &xSystemMonitorTaskHandle);
}
