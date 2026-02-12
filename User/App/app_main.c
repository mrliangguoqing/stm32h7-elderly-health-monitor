#include "app_main.h"

#include "FreeRTOS.h"
#include "task.h"

#include "app_display.h"
#include "app_system_monitor.h"

/**
 * @brief  初始化 APP 创建任务，启动调度器
 * @param  None
 * @retval None
 */
void APP_Init(void)
{
    /* 调用各 APP 模块的初始化接口 */
    App_Display_Init();
    App_System_Monitor_Init();

    /* 启动调度器 */
    vTaskStartScheduler();

    while (1);
}