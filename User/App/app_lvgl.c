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

#include "app_rtc_alarm.h"
#include "pal_log.h"

#include "bsp_lcd.h"
#include "bsp_gt911.h"
#include "bsp_ds1302.h"
#include "bsp_aht30.h"
#include "bsp_bh1750.h"
#include "bsp_max30102.h"

#include "utils_math.h"

#include <stdio.h>

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xLvglTaskHandle = NULL;

void alarm_ui_initial_state(void);
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
    alarm_ui_initial_state();                              /* 更新闹钟开关 UI */
    lv_obj_set_parent(ui_PanelAlarmPopup, lv_layer_top()); /* 弹窗设置到最顶层 */

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
                4096,
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

void alarm_ui_initial_state(void)
{
    if (g_user_alarm.is_enabled)
    {
        lv_obj_add_state(ui_SwitchAlarm, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_clear_state(ui_SwitchAlarm, LV_STATE_CHECKED);
    }
}

void update_rtc_time_cb(lv_timer_t *timer)
{
    ds1302_data_t *p_ds1302_data = BSP_DS1302_GetData();
    const char *week_map[] = {"日", "一", "二", "三", "四", "五", "六"};

    if (g_user_alarm.is_active)
    {
        /* 检查当前是否已经是显示状态，避免重复清除 Hidden 标志 */
        if (lv_obj_has_flag(ui_PanelAlarmPopup, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_clear_flag(ui_PanelAlarmPopup, LV_OBJ_FLAG_HIDDEN);
            PAL_LOG(PAL_LOG_LEVEL_INFO, "UI Alarm Popup Triggered!");
        }
    }

    lv_label_set_text_fmt(ui_LabelTime, "%02d:%02d:%02d", p_ds1302_data->hour, p_ds1302_data->minute, p_ds1302_data->second);
    lv_label_set_text_fmt(ui_LabelYear, "%04d", p_ds1302_data->year);
    lv_label_set_text_fmt(ui_LabelMonth, "%02d", p_ds1302_data->month);
    lv_label_set_text_fmt(ui_LabelDay, "%02d", p_ds1302_data->day);
    lv_label_set_text_fmt(ui_LabelWeek, "周%s", week_map[p_ds1302_data->week]);

    /* 屏幕 2 */
    lv_label_set_text_fmt(ui_LabelTime2, "%02d:%02d:%02d", p_ds1302_data->hour, p_ds1302_data->minute, p_ds1302_data->second);

    /* 屏幕 3 */
    lv_label_set_text_fmt(ui_LabelTime3, "%02d:%02d:%02d", p_ds1302_data->hour, p_ds1302_data->minute, p_ds1302_data->second);

    /* 屏幕 4 */
    lv_label_set_text_fmt(ui_LabelTime4, "%02d:%02d:%02d", p_ds1302_data->hour, p_ds1302_data->minute, p_ds1302_data->second);

    /* 屏幕 5 */
    lv_label_set_text_fmt(ui_LabelTime5, "%02d:%02d:%02d", p_ds1302_data->hour, p_ds1302_data->minute, p_ds1302_data->second);
}

/**
 * @brief 辅助函数：更新带有 1 位小数标签的 Slider
 */
void ui_helper_set_slider_with_1dec(lv_obj_t *slider, lv_obj_t *label, float value)
{
    int32_t v_int = 0;
    uint32_t v_dec = 0;

    UTILS_Math_SplitFloat(value, 1, &v_int, &v_dec, NULL);

    /* 更新 UI */
    lv_slider_set_value(slider, v_int, LV_ANIM_ON);
    lv_label_set_text_fmt(label, "%d.%d", v_int, v_dec);
}

/**
 * @brief  更新心率血氧界面 UI
 * @param  p_max30102: 指向 MAX30102 设备句柄的指针
 */
void ui_update_health_screen(const max30102_handle_t *p_max30102)
{
    const max30102_data_t *p_data = &(p_max30102->data);
    health_status_t current_status;

    /* 根据结构体数值的判定健康检测的状态 */
    if (p_data->heart_rate_valid && p_data->spo2_valid)
    {
        current_status = HEALTH_STATUS_READY;
    }
    else if (p_data->heart_rate == HEART_RATE_DETECTING)
    {
        current_status = HEALTH_STATUS_DETECTING;
    }
    else
    {
        current_status = HEALTH_STATUS_OFF_FINGER;
    }

    /* 根据状态更新提示词，刷新 UI */
    switch (current_status)
    {
    case HEALTH_STATUS_OFF_FINGER:
        lv_label_set_text(ui_LabelHealthStatus, "请将手指平放于传感器上");
        lv_obj_set_style_text_color(ui_LabelHealthStatus, lv_color_hex(0xEEEEEE), 0);

        /* 数据无效时，数值显示为 "--" */
        lv_label_set_text(ui_LabelHeartNum, "--");
        lv_label_set_text(ui_LabelSpO2Num, "--");
        lv_arc_set_value(ui_ArcHeart, 40); /* 回到最小值 */
        lv_arc_set_value(ui_ArcSpO2, 80);  /* 回到最小值 */
        break;

    case HEALTH_STATUS_DETECTING:
        lv_label_set_text(ui_LabelHealthStatus, "正在检测中...");
        lv_obj_set_style_text_color(ui_LabelHealthStatus, lv_palette_main(LV_PALETTE_BLUE), 0);

        /* 数据不稳定时，数值显示为 "--" */
        lv_label_set_text(ui_LabelHeartNum, "--");
        lv_label_set_text(ui_LabelSpO2Num, "--");
        break;

    case HEALTH_STATUS_READY:
        lv_label_set_text(ui_LabelHealthStatus, "实时监测中");
        lv_obj_set_style_text_color(ui_LabelHealthStatus, lv_palette_main(LV_PALETTE_GREEN), 0);

        /* 更新心率 Arc 和 Label */
        lv_arc_set_value(ui_ArcHeart, (int)p_data->stable_heart_rate);
        lv_label_set_text_fmt(ui_LabelHeartNum, "%d", (int)p_data->stable_heart_rate);

        /* 更新血氧 Arc 和 Label */
        lv_arc_set_value(ui_ArcSpO2, (int)p_data->spo2);
        lv_label_set_text_fmt(ui_LabelSpO2Num, "%d", (int)p_data->spo2);

        /* 动态颜色预警逻辑 (血氧低于 94% 变色) */
        if (p_data->spo2 < 94)
        {
            lv_obj_set_style_arc_color(ui_ArcSpO2, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
        }
        else
        {
            lv_obj_set_style_arc_color(ui_ArcSpO2, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
        }
        break;
    }
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

    /* 更新心率血氧 */
    ui_update_health_screen(&max30102_handle);
}
