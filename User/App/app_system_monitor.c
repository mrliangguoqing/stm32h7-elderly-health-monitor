/**
 * @file    app_system_monitor.c
 * @brief   应用层系统监视模块
 * @details 负责监控 FreeRTOS 任务的情况，通过串口打印信息
 */

#include "app_system_monitor.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdint.h>
#include "debug.h"

#if (APP_SYSTEM_MONITOR_ENABLE == 1)

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xSystemMonitorTaskHandle = NULL;

/* 高频计时器计数值，由硬件定时器（建议 50us-100us 周期）在中断回调函数中累加 */
volatile uint32_t ulHighFrequencyTimerTicks = 0UL;

/**
 * @brief  系统监视处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void System_Monitor_Task(void *pvParameters)
{
    /* 任务状态统计缓冲区 */
    static char pcWriteBuffer[1024];

    for (;;)
    {
        memset(pcWriteBuffer, 0, strlen((char *)pcWriteBuffer));
        printf("=================================================\r\n");
        printf("任务名      任务状态 优先级   剩余栈 任务序号\r\n");
        vTaskList((char *)&pcWriteBuffer);
        printf("%s\r\n", pcWriteBuffer);

        memset(pcWriteBuffer, 0, strlen((char *)pcWriteBuffer));
        printf("\r\n任务名       运行计数         使用率\r\n");
        vTaskGetRunTimeStats((char *)&pcWriteBuffer);
        printf("%s\r\n", pcWriteBuffer);

        vTaskDelay(pdMS_TO_TICKS(1000));
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
                2,
                &xSystemMonitorTaskHandle);
}

#endif
