/**
 * @file    app_rtc_alarm.c
 * @brief   应用层实时时钟、闹钟逻辑模块
 */

#include "app_rtc_alarm.h"

#include "FreeRTOS.h"
#include "task.h"

#include "bsp_ds1302.h"

#include "pal_log.h"

/* 全局闹钟实例 */
alarm_config_t g_user_alarm = {8, 0, false, false};

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xRtcAlarmTaskHandle = NULL;

/**
 * @brief  实时时钟、闹钟处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void RTC_Alarm_Task(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); /* 恒定 1 秒触发一次 */
    ds1302_data_t *p_ds1302_data = BSP_DS1302_GetData();

    /* 初始化时间点 */
    xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        /* 绝对延时，确保每秒准时唤醒 */
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        /* 读取硬件 RTC 时间 */
        BSP_DS1302_UpdateData(p_ds1302_data);

        /* 闹钟判断逻辑 */
        if (g_user_alarm.is_enabled && !g_user_alarm.is_active)
        {
            if (p_ds1302_data->hour == g_user_alarm.hour &&
                p_ds1302_data->minute == g_user_alarm.minute &&
                p_ds1302_data->minute != g_user_alarm.last_alarm_min) /* 这一分钟没响过 */
            {

                g_user_alarm.is_active = true;                       /* 标记闹钟已触发 */
                g_user_alarm.last_alarm_min = p_ds1302_data->minute; /* 记录上一次响铃的分钟 */
            }
        }

        PAL_LOG(PAL_LOG_LEVEL_DEBUG, "Date: %04d-%02d-%02d  Time: %02d:%02d:%02d  Week: %d",
                p_ds1302_data->year, p_ds1302_data->month, p_ds1302_data->day,
                p_ds1302_data->hour, p_ds1302_data->minute, p_ds1302_data->second,
                p_ds1302_data->week);
    }
}

/**
 * @brief  实时时钟、闹钟模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_RTC_Alarm_Init(void)
{
    /* BSP 层模块初始化 */
    BSP_DS1302_Init();

    xTaskCreate(RTC_Alarm_Task,
                "RTC_Alarm_Task",
                512,
                NULL,
                4,
                &xRtcAlarmTaskHandle);
}
