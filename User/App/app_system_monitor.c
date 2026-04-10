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

#include "pal_log.h"

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
    /* 建议加大缓冲区或确保任务数量在受控范围 */
    static char pcWriteBuffer[1024];

    for (;;)
    {
        memset(pcWriteBuffer, 0, sizeof(pcWriteBuffer));

        printf("\r\n================ System Monitor ================\r\n");
        printf("TaskName\tStatus\tPrio\tStack\tNum\r\n");

        vTaskList(pcWriteBuffer);
        printf("%s", pcWriteBuffer);

        printf("------------------------------------------------\r\n");
        printf("TaskName\tRunCount\tUsage\r\n");

        memset(pcWriteBuffer, 0, sizeof(pcWriteBuffer));
        vTaskGetRunTimeStats(pcWriteBuffer);
        printf("%s", pcWriteBuffer);

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
                512,
                NULL,
                14,
                &xSystemMonitorTaskHandle);
}

/* 栈溢出钩子函数 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("Stack overflow in task: %s\r\n", pcTaskName);
    while (1);
}

#endif
