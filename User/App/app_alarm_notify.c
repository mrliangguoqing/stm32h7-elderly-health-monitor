/**
 * @file    app_alarm_notify.c
 * @brief   应用层报警通知逻辑模块
 * @note    此模块的任务拥有最高优先级
 */

#include "app_alarm_notify.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "bsp_a7670c.h"

#include "pal_log.h"

/* 定义二值信号量，用于触发报警通知 */
SemaphoreHandle_t xAlarmSemaphore = NULL;

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xAlarmNotifyTaskHandle = NULL;

/* 5分钟的冷却时间（单位：FreeRTOS Tick） */
#define ALARM_COOLDOWN_TIME pdMS_TO_TICKS(5 * 60 * 1000)

/* TODO: 添加触发报警通知的方式，暂定为语音求救指令+LVGL 求救按钮的形式 */

// /* 释放信号量，唤醒报警任务 */
// if (xAlarmSemaphore != NULL)
// {
//     xSemaphoreGive(xAlarmSemaphore);
// }

/**
 * @brief  报警通知处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Alarm_Notify_Task(void *pvParameters)
{
    TickType_t xLastSendTick = 0; /* 记录上次发送成功的时间点 */

    for (;;)
    {
        /* 等待信号量 */
        if (xSemaphoreTake(xAlarmSemaphore, portMAX_DELAY) == pdPASS)
        {
            TickType_t xCurrentTick = xTaskGetTickCount();

            /* 检查是否在 5 分钟冷却期内 */
            if ((xLastSendTick == 0) || (xCurrentTick - xLastSendTick >= ALARM_COOLDOWN_TIME))
            {
                PAL_LOG(PAL_LOG_LEVEL_INFO, "Condition met, sending emergency SMS...");

                if (BSP_A7670C_SendEmergencySMS() == 0) /* 调用发送紧急报警 API */
                {
                    xLastSendTick = xCurrentTick; /* 更新最后发送时间 */
                    PAL_LOG(PAL_LOG_LEVEL_INFO, "SMS sent. Cooldown started.");
                }
            }
            else
            {
                PAL_LOG(PAL_LOG_LEVEL_ERROR, "Alarm triggered but ignored due to 5-min cooldown.");
            }
        }
    }
}

/**
 * @brief  报警通知模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_Alarm_Notify_Init(void)
{

    /* 创建信号量，用于外部中断回调函数中通知采集任务 */
    if (xAlarmSemaphore == NULL)
    {
        xAlarmSemaphore = xSemaphoreCreateBinary();
    }

    xTaskCreate(Alarm_Notify_Task,
                "Alarm_Notify_Task",
                256,
                NULL,
                6,
                &xAlarmNotifyTaskHandle);
}
