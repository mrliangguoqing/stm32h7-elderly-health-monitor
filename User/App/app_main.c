/**
 * @file    app_main.c
 * @brief   应用层系统入口与任务协调模块
 * @details 负责统一创建应用层所有 FreeRTOS 任务、同步对象，并最终启动内核调度器。
 * 通过 app_config.h 中的宏定义实现功能的模块化裁剪。
 */

#include "app_main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_config.h"
#include "app_display.h"
#include "app_system_monitor.h"

/**
 * @brief  系统应用层总初始化
 * @note   该函数应在硬件 BSP 初始化完成后，在 main 函数的末尾被调用。
 * 调用后将进入 FreeRTOS 任务调度循环，不再返回。
 */
void APP_Init(void)
{
    /* 调用各 APP 模块的初始化接口 */
    App_Display_Init();

#if (APP_SYSTEM_MONITOR_ENABLE == 1)
    App_System_Monitor_Init();
#endif

    /* 启动调度器 */
    vTaskStartScheduler();

    while (1);
}