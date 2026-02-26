/**
 * @file    app_display.c
 * @brief   应用层显示逻辑模块
 * @details 负责处理屏幕刷新、UI逻辑，通过调用 BSP 层 LCD 驱动实现界面显示
 */

#include "app_display.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_lcd.h"

#include "lvgl.h"
#include "lv_port_disp_template.h"
#include "lv_demo_widgets.h"

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xDisplayTaskHandle = NULL;

/**
 * @brief  屏幕显示处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Display_Task(void *pvParameters)
{
    
    
    lv_init();  /* 初始化 LVGL 核心库 */
    lv_port_disp_init(); /* 初始化显示接口 */

//    lv_obj_t * btn = lv_button_create(lv_screen_active());
//    lv_obj_set_size(btn, 150, 50);
//    lv_obj_center(btn);
//    lv_obj_t * label = lv_label_create(btn);
//    lv_label_set_text(label, "Hello STM32!");
//    lv_obj_center(label);
    
#if LV_USE_DEMO_WIDGETS
    lv_demo_benchmark(); 
#endif

    for (;;)
    {
        uint32_t time_till_next = lv_timer_handler();
        
        /* time_till_next 是 LVGL 建议的下次执行时间，通常在 1-30ms 之间 */
        if(time_till_next < 5) time_till_next = 5; 
        vTaskDelay(pdMS_TO_TICKS(time_till_next));
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
                2048,
                NULL,
                1,
                &xDisplayTaskHandle);
}
