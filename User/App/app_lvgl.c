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
#include "bsp_aht30.h"
#include "bsp_bh1750.h"
#include "bsp_max30102.h"

#include <stdio.h>

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xLvglTaskHandle = NULL;

void update_rtc_time_cb(lv_timer_t *timer);
void update_sensor_data_cb(lv_timer_t *timer);

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

    lv_timer_create(update_rtc_time_cb, 1000, NULL);   /* 1秒更新一次时间 */
    lv_timer_create(update_sensor_data_cb, 500, NULL); // 0.5秒更新一次温湿度

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

void update_rtc_time_cb(lv_timer_t *timer)
{
    ds1302_data_t *p_ds1302_data = BSP_DS1302_GetData();
    const char *week_map[] = {"日", "一", "二", "三", "四", "五", "六"};

    lv_label_set_text_fmt(ui_LabelTime, "%02d:%02d:%02d", p_ds1302_data->hour, p_ds1302_data->minute, p_ds1302_data->second);
    lv_label_set_text_fmt(ui_LabelYear, "%04d", p_ds1302_data->year);
    lv_label_set_text_fmt(ui_LabelMonth, "%02d", p_ds1302_data->month);
    lv_label_set_text_fmt(ui_LabelDay, "%02d", p_ds1302_data->day);
    lv_label_set_text_fmt(ui_LabelWeek, "周%s", week_map[p_ds1302_data->week]);
}

/**
 * @brief 辅助函数：更新带有 1 位小数标签的 Slider
 */
void ui_helper_set_slider_with_1dec(lv_obj_t *slider, lv_obj_t *label, float value)
{
    /* 四舍五入到 1 位小数 */
    float rounded = value + 0.05f;
    int v_int = (int)rounded;
    int v_dec = (int)((rounded - v_int) * 10);

    /* 确保小数部分始终为正数 */
    if (v_dec < 0)
    {
        v_dec = -v_dec;
    }

    /* 更新 UI */
    lv_slider_set_value(slider, v_int, LV_ANIM_ON);
    lv_label_set_text_fmt(label, "%d.%d", v_int, v_dec);
}

void update_sensor_data_cb(lv_timer_t *timer)
{

    const aht30_data_t *p_aht30_data = BSP_AHT30_GetData();
    const bh1750_data_t *p_bh1750_data = BSP_BH1750_GetData();

    ui_helper_set_slider_with_1dec(ui_SliderTemp, ui_LabelTempNum, p_aht30_data->temperature); /* 更新温度 */
    ui_helper_set_slider_with_1dec(ui_SliderHumi, ui_LabelHumiNum, p_aht30_data->humidity);    /* 更新湿度 */

    if (p_aht30_data->temperature > 30.0f)
    {
        /* 如果温度太高，把 Slider 的颜色变红 */
        lv_obj_set_style_bg_color(ui_SliderTemp, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    }
    else
    {
        /* 正常时为蓝色或绿色 */
        lv_obj_set_style_bg_color(ui_SliderTemp, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    }

    /* 更新光照 */
    lv_arc_set_value(ui_ArcLight, (int)p_bh1750_data->lux);
    lv_label_set_text_fmt(ui_LabelLightNum, "%d", (int)p_bh1750_data->lux);

    //   char buf[128];

    //   if(max30102_handle.data.heart_rate_valid == 1 && max30102_handle.data.spo2_valid == 1)
    //   {
    //       snprintf(buf, sizeof(buf), "心率: %02d 血氧: %02d %%", max30102_handle.data.stable_heart_rate,max30102_handle.data.spo2);
    //   }
    //   else
    //   {
    //       snprintf(buf, sizeof(buf), "心率: %02d bpm 血氧: %02d %%", 0,0);
    //   }
    //

    //   /* 更新 UI 组件 */
    //   if (ui_Illumination != NULL)
    //   {
    //       lv_label_set_text(ui_Illumination, buf);
    //   }
}