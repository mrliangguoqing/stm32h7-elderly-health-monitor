/**
 * @file    app_sync_netdata.c
 * @brief   应用层同步网络数据逻辑模块
 */

#include "app_sync_netdata.h"

#include "FreeRTOS.h"
#include "task.h"

#include "bsp_ds1302.h"
#include "bsp_esp8266.h"

#include "pal_log.h"

#include <stdbool.h>

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xSyncNetdataTaskHandle = NULL;

static uint8_t Sync_Weather(void);
static uint8_t Sync_Time(ds1302_data_t *p_ds1302);

/**
 * @brief  同步网络数据处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Sync_Netdata_Task(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(3600 * 1000);
    uint8_t hour_counter = 0;
    uint8_t retry_count = 0;
    const uint8_t MAX_RETRY = 5; /* 最大重试次数 */

    ds1302_data_t *p_ds1302_data = BSP_DS1302_GetData();

    BSP_ESP8266_Init();

    /* 初始强制同步 */
    vTaskPrioritySet(NULL, 11);
    uint8_t weather_err = 1; /* 默认失败状态 */
    uint8_t time_err = 1;

    while ((weather_err || time_err) && (retry_count < MAX_RETRY))
    {
        /* 如果之前失败了(1)，则尝试同步；如果已经成功(0)，则跳过 */
        if (weather_err)
        {
            weather_err = Sync_Weather();
        }

        if (time_err)
        {
            time_err = Sync_Time(p_ds1302_data);
        }

        if (weather_err || time_err) /* 如果还有任何一个没成功，就等待重试 */
        {
            retry_count++;
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
    vTaskPrioritySet(NULL, 2);

    /* 周期性同步 */
    xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency); /* 绝对延时 */

        Sync_Weather(); /* 每 1 小时同步天气 */

        if (hour_counter % 12 == 0) /* 每 12 小时同步时间 */
        {
            Sync_Time(p_ds1302_data);
        }

        if (++hour_counter >= 12)
        {
            hour_counter = 0;
        }
    }
}

/**
 * @brief  同步网络数据模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_Sync_Netdata_Init(void)
{
    xTaskCreate(Sync_Netdata_Task,
                "Sync_Netdata_Task",
                512,
                NULL,
                2,
                &xSyncNetdataTaskHandle);
}

/**
 * @brief  执行天气同步
 * @return 0: 成功, 1: 失败
 */
static uint8_t Sync_Weather(void)
{
    if (BSP_ESP8266_WeatherUpdate(&g_weather_info) == 0)
    {
        BSP_ESP8266_Weather_Print(&g_weather_info);

        return 0;
    }

    PAL_LOG(PAL_LOG_LEVEL_ERROR, "请求天气数据失败\r\n");

    return 1;
}

/**
 * @brief  执行时间同步并更新硬件 RTC
 * @return 0: 成功, 1: 失败
 */
static uint8_t Sync_Time(ds1302_data_t *p_ds1302)
{
    if (BSP_ESP8266_SyncTime(&g_net_time) == 0)
    {
        p_ds1302->year = g_net_time.year;
        p_ds1302->month = g_net_time.month;
        p_ds1302->day = g_net_time.day;
        p_ds1302->hour = g_net_time.hour;
        p_ds1302->minute = g_net_time.minute;
        p_ds1302->second = g_net_time.second;
        p_ds1302->week = g_net_time.week;

        BSP_DS1302_SetTime(p_ds1302);
        BSP_ESP8266_Time_Print(&g_net_time);

        return 0;
    }

    PAL_LOG(PAL_LOG_LEVEL_ERROR, "请求网络时间失败\r\n");

    return 1;
}
