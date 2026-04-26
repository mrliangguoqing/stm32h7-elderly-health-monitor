/**
 * @file    app_peripheral.c
 * @brief   外围器件逻辑模块
 */

#include "app_peripheral.h"

#include "FreeRTOS.h"
#include "task.h"

#include "bsp_ws2812b.h"

#include "pal_log.h"

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xPeripheralTaskHandle = NULL;

/**
 * @brief  外围器件处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Peripheral_Task(void *pvParameters)
{
    static uint16_t start_hue = 0, hue = 0;

    for (;;)
    {
        // for (int i = 0; i < WS2812B_QUANTITY; i++)
        // {
        //     BSP_WS2812_SetHSV(i, (float)0, 0, 1.0f);
        // }
        // BSP_WS2812_Update();

        for (int i = 0; i < WS2812B_QUANTITY; i++)
        {
            /* 每个灯珠的色调比前一个偏移 20 度 */
            uint16_t hue = (start_hue + (i * 20)) % 360;
            BSP_WS2812_SetHSV(i, (float)hue, 1.0f, 0.1f);
        }
        BSP_WS2812_Update();
        start_hue = (start_hue + 5) % 360; /* 偏移起始色调，产生流动感 */

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

/**
 * @brief  外围器件初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_Peripheral_Init(void)
{
    /* BSP 层模块初始化 */

    xTaskCreate(Peripheral_Task,
                "Peripheral",
                512,
                NULL,
                5,
                &xPeripheralTaskHandle);
}
