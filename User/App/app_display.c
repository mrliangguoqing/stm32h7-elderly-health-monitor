/**
 * @file    app_display.c
 * @brief   应用层显示逻辑模块
 * @details 负责处理屏幕刷新、UI逻辑，通过调用 BSP 层 LCD 驱动实现界面显示
 */

#include "app_display.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_lcd.h"

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xDisplayTaskHandle = NULL;

/**
 * @brief  屏幕显示处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Display_Task(void *pvParameters)
{
    for (;;)
    {
        LCD_Clear(BLUE);
        vTaskDelay(pdMS_TO_TICKS(20));
        LCD_Clear(BLACK);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/**
 * @brief  显示模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_Display_Init(void)
{
    xTaskCreate(Display_Task,
                "Display_Task",
                256,
                NULL,
                2,
                &xDisplayTaskHandle);
}