/**
 * @file    app_main.c
 * @brief   应用层系统入口与任务协调模块
 * @details 负责统一创建应用层所有 FreeRTOS 任务、同步对象，并最终启动内核调度器。
 * 通过 app_config.h 中的宏定义实现功能的模块化裁剪。
 */

#include "app_main.h"
#include "app_config.h"
#include "app_lvgl.h"
#include "app_sensor.h"
#include "app_rtc_alarm.h"
#include "app_max30102.h"
#include "app_alarm_notify.h"
#include "app_sync_netdata.h"
#include "app_voice_service.h"
#include "app_system_monitor.h"

#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief  系统应用层总初始化
 * @note   该函数应在硬件 BSP 初始化完成后，在 main 函数的末尾被调用。
 * 调用后将进入 FreeRTOS 任务调度循环，不再返回。
 */
void APP_Init(void)
{

#if (APP_SYSTEM_MONITOR_ENABLE == 1)
    App_System_Monitor_Init();
#endif

    /* 调用各 APP 模块的初始化接口 */
    // App_Sync_Netdata_Init(); /* 同步网络数据任务 */
    App_RTC_Alarm_Init(); /* 实时时钟、闹钟任务 */
    App_Sensor_Init();    /* 传感器采集任务 */
    App_Alarm_Notify_Init(); /* 报警通知任务 */
    App_Lvgl_Init();     /* LVGL 任务 */

    App_Voice_Service_Init();

    // App_Max30102_Init(); /* MAX30102 采集、算法任务 */

    /* 启动调度器 */
    vTaskStartScheduler();

    for (;;)
    {
    }
}