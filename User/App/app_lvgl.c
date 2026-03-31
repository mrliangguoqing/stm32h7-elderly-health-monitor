/**
 * @file    app_lvgl.c
 * @brief   应用层 LVGL 显示触摸逻辑模块
 */

#include "app_lvgl.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

#include "ui.h"

#include "bsp_lcd.h"
#include "bsp_gt911.h"
#include "bsp_ds1302.h"

#include <stdio.h>

void update_rtc_time_cb(lv_timer_t *timer);

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xLvglTaskHandle = NULL;

/**
 * @brief  LVGL 处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Lvgl_Task(void *pvParameters)
{
    lv_init();            /* 初始化 LVGL 核心库 */
    lv_port_disp_init();  /* 初始化显示接口 */
    lv_port_indev_init(); /* 初始化触摸接口 */

    ui_init();

    /* 创建定时器，每 1000ms (1秒) 执行一次模拟更新 */
    lv_timer_create(update_rtc_time_cb, 1000, NULL);

    for (;;)
    {
        uint32_t time_till_next = lv_timer_handler();

        /* time_till_next 是 LVGL 建议的下次执行时间，通常在 1-30ms 之间 */
        if (time_till_next < 5)
        {
            time_till_next = 5;
        }

        vTaskDelay(pdMS_TO_TICKS(time_till_next));
    }
}

/**
 * @brief  LVGL 模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_Lvgl_Init(void)
{
    /* BSP 层模块初始化 */
    BSP_GT911_Init();
    BSP_LCD_Init();
    BSP_GT911_BindLCD();

    xTaskCreate(Lvgl_Task,
                "Lvgl_Task",
                2048,
                NULL,
                8,
                &xLvglTaskHandle);
}

/**
 * @brief  FreeRTOS 系统滴答定时器钩子函数 (Tick Hook)
 * @note   该函数在 FreeRTOS 的每个 Tick 中断中被自动调用
 *         使用此函数需要在 FreeRTOSConfig.h 中开启 configUSE_TICK_HOOK
 */
void vApplicationTickHook(void)
{
    /* 告诉 LVGL 过去了 1 毫秒，以便 LVGL 能够准确计算时间，处理内部的计时器和任务 */
    lv_tick_inc(1);
}

/* 模拟时钟回调函数 */
void update_rtc_time_cb(lv_timer_t *timer)
{
    char buf[32];

    ds1302_data_t *p_ds1302_data = BSP_DS1302_GetData();

    snprintf(buf, sizeof(buf), "Time: %02d:%02d:%02d", p_ds1302_data->hour, p_ds1302_data->minute, p_ds1302_data->second);

    /* 更新 UI 组件 */
    if (ui_LabelTime != NULL)
    {
        lv_label_set_text(ui_LabelTime, buf);
    }
}
