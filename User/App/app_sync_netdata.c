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

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xSyncNetdataTaskHandle = NULL;

/**
 * @brief  同步网络数据处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Sync_Netdata_Task(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(3600 * 1000); /* 恒定 1 小时触发一次 */
    static uint8_t hour_counter = 0;                          /* 小时计数器 */

    ds1302_data_t *p_ds1302_data = BSP_DS1302_GetData();

    uint8_t status = 0x00;
    static uint8_t num = 0;

    /* BSP 层模块初始化 */
    BSP_ESP8266_Init();

    vTaskPrioritySet(NULL, 11);

    do
    {
        status = 0x00;

        PAL_LOG(PAL_LOG_LEVEL_ERROR, "请求网络同步次数 ------ %d", num);
        num++;

        if (BSP_ESP8266_WeatherUpdate(&g_weather_info) == 0)
        {
            status |= 0xF0;
            BSP_ESP8266_Weather_Print(&g_weather_info);
        }
        else
        {
            PAL_LOG(PAL_LOG_LEVEL_ERROR, "请求天气数据失败\r\n");
        }

        /* 更新时间到 DS1302 */
        if (BSP_ESP8266_SyncTime(&g_net_time) == 0)
        {
            status |= 0x0F;
            p_ds1302_data->year = g_net_time.year;
            p_ds1302_data->month = g_net_time.month;
            p_ds1302_data->day = g_net_time.day;
            p_ds1302_data->hour = g_net_time.hour;
            p_ds1302_data->minute = g_net_time.minute;
            p_ds1302_data->second = g_net_time.second;
            p_ds1302_data->week = g_net_time.week;

            BSP_DS1302_SetTime(p_ds1302_data);

            BSP_ESP8266_Time_Print(&g_net_time);
        }
        else
        {
            PAL_LOG(PAL_LOG_LEVEL_ERROR, "RTC_Alarm_Task 请求网络时间失败");
        }
    } while (status != 0xFF);

    vTaskPrioritySet(NULL, 2);

    /* 初始化时间点 */
    xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        /* 绝对延时，确保每 1 小时准时唤醒 */
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        /* 每 1 小时更新同步天气数据 */
        if (BSP_ESP8266_WeatherUpdate(&g_weather_info) == 0)
        {
            BSP_ESP8266_Weather_Print(&g_weather_info);
        }
        else
        {
            PAL_LOG(PAL_LOG_LEVEL_ERROR, "请求天气数据失败\r\n");
        }

        /* 每 12 小时更新同步时间日期数据 */
        if (hour_counter % 12 == 0)
        {
            if (BSP_ESP8266_SyncTime(&g_net_time) == 0)
            {
                BSP_ESP8266_Time_Print(&g_net_time);
            }
            else
            {
                PAL_LOG(PAL_LOG_LEVEL_ERROR, "请求网络时间失败\r\n");
            }
        }

        hour_counter++;
        if (hour_counter >= 12)
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
