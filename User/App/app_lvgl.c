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
#include "bsp_esp8266.h"

#include "utils_math.h"

#include <stdio.h>

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xLvglTaskHandle = NULL;

typedef struct
{
    int code;                      /* 天气代码 */
    const lv_image_dsc_t *img_src; /* 图片资源指针 */
    const char *desc;              /* 天气描述文字 */
} Weather_Data_Map_t;

/* 天气映射表 */
const Weather_Data_Map_t weather_master_map[] = {
    {0, &ui_img_weather_code0_sunny_png, "晴"},
    {1, &ui_img_weather_code1_clear_png, "晴"},
    {2, &ui_img_weather_code0_sunny_png, "晴"},
    {3, &ui_img_weather_code1_clear_png, "晴"},
    {4, &ui_img_weather_code4_cloudy_png, "多云"},
    {5, &ui_img_weather_code5_partly_cloudy_png, "晴间多云"},
    {6, &ui_img_weather_code6_partly_cloudy_png, "晴间多云"},
    {7, &ui_img_weather_code7_mostly_cloudy_png, "大部多云"},
    {8, &ui_img_weather_code8_mostly_cloudy_png, "大部多云"},
    {9, &ui_img_weather_code9_overcast_png, "阴"},
    {10, &ui_img_weather_code10_shower_png, "阵雨"},
    {11, &ui_img_weather_code11_thundershower_png, "雷阵雨"},
    {12, &ui_img_weather_code12_thundershower_with_hail_png, "雷阵雨伴有冰雹"},
    {13, &ui_img_weather_code13_light_rain_png, "小雨"},
    {14, &ui_img_weather_code14_moderate_rain_png, "中雨"},
    {15, &ui_img_weather_code15_heavy_rain_png, "大雨"},
    {16, &ui_img_weather_code16_storm_png, "暴雨"},
    {17, &ui_img_weather_code17_heavy_storm_png, "大暴雨"},
    {18, &ui_img_weather_code18_severe_storm_png, "特大暴雨"},
    {19, &ui_img_weather_code19_ice_rain_png, "冻雨"},
    {20, &ui_img_weather_code20_sleet_png, "雨夹雪"},
    {21, &ui_img_weather_code21_snow_flurry_png, "阵雪"},
    {22, &ui_img_weather_code22_light_snow_png, "小雪"},
    {23, &ui_img_weather_code23_moderate_snow_png, "中雪"},
    {24, &ui_img_weather_code24_heavy_snow_png, "大雪"},
    {25, &ui_img_weather_code25_snowstorm_png, "暴雪"},
    {26, &ui_img_weather_code26_dust_png, "浮尘"},
    {27, &ui_img_weather_code27_sand_png, "扬沙"},
    {28, &ui_img_weather_code28_duststorm_png, "沙尘暴"},
    {29, &ui_img_weather_code29_sandstorm_png, "强沙尘暴"},
    {30, &ui_img_weather_code30_foggy_png, "雾"},
    {31, &ui_img_weather_code31_haze_png, "霾"},
    {32, &ui_img_weather_code32_windy_png, "风"},
    {33, &ui_img_weather_code33_blustery_png, "大风"},
    {34, &ui_img_weather_code34_hurricane_png, "飓风"},
    {35, &ui_img_weather_code35_tropical_storm_png, "热带风暴"},
    {36, &ui_img_weather_code36_tornado_png, "龙卷风"},
    {37, &ui_img_weather_code37_cold_png, "冷"},
    {38, &ui_img_weather_code0_sunny_png, "热"},
    {99, &ui_img_weather_code99_unknown_png, "未知"}};

#define WEATHER_MASTER_COUNT (sizeof(weather_master_map) / sizeof(weather_master_map[0]))

void alarm_ui_initial_state(void);
void update_rtc_time_cb(lv_timer_t *timer);
void update_sensor_data_cb(lv_timer_t *timer);
void ui_update_weather_display(const WeatherInfo *info);
void ui_update_weather_info(lv_obj_t *img_obj, lv_obj_t *label_obj, int code);

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

    if (timer == NULL || p_ds1302_data == NULL)
    {
        return;
    }

    const char *week_map[] = {"Invalid", "一", "二", "三", "四", "五", "六", "日"};
    uint8_t w_idx = p_ds1302_data->week;

    if (w_idx < 1 || w_idx > 7) /* 合法性校验 */
    {
        w_idx = 0; /* 指向 "Invalid" */
    }

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
    lv_label_set_text_fmt(ui_LabelWeek, "周%s", week_map[w_idx]);

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

    /* 更新天气 */
    ui_update_weather_display(&g_weather_info);
}

/**
 * @brief  更新 UI 界面上的天气数据
 * @param  info 从 ESP8266 接收并解析后的天气结构体
 */
void ui_update_weather_display(const WeatherInfo *info)
{
    if (info == NULL)
    {
        return;
    }

    /* 定义临时变量用于浮点数拆分 */
    int32_t i_part;
    uint32_t d_part;
    int8_t sign;

    /* 循环更新 TabView 中的三天数据 */
    for (int i = 0; i < 3 && i < info->daily_count; i++)
    {
        const WeatherDay *d = &info->daily[i];

        if (i == 0)
        {
            /* 第一天 */
            /* 天气图标 天气描述 */
            ui_update_weather_info(ui_ImageWeather, ui_LabelWeatherDesc, d->code_day);

            /* 最低温度 最高温度 */
            lv_label_set_text_fmt(ui_LabelTempMinNUM, "%d", d->low);
            lv_label_set_text_fmt(ui_LabelTempMaxNUM, "%d", d->high);

            /* 降雨量 */
            UTILS_Math_SplitFloat(d->rainfall, 2, &i_part, &d_part, &sign);
            lv_label_set_text_fmt(ui_LabelRainFallVal, "%d.%02u", (int)i_part, (unsigned int)d_part);

            /* 降水概率 */
            lv_label_set_text_fmt(ui_LabelRainProbVal, "%d", (int)d->precip);

            /* 风向 角度 */
            lv_label_set_text(ui_LabelWindDir, d->wind_direction);
            lv_label_set_text_fmt(ui_LabelWindAngle, "%d", d->wind_direction_degree);

            /* 风速 */
            UTILS_Math_SplitFloat(d->wind_speed, 1, &i_part, &d_part, &sign);
            lv_label_set_text_fmt(ui_LabelWindSpeedVal, "%d.%u", (int)i_part, (unsigned int)d_part);

            lv_label_set_text_fmt(ui_LabelWindLevelVal, "%d", d->wind_scale);
            lv_label_set_text_fmt(ui_LabelHumVal, "%d", d->humidity);
        }
        else if (i == 1)
        {
            /* 第二天 */
            ui_update_weather_info(ui_ImageWeather1, ui_LabelWeatherDesc1, d->code_day);

            lv_label_set_text_fmt(ui_LabelTempMaxNUM1, "%d", d->high);
            lv_label_set_text_fmt(ui_LabelTempMinNUM1, "%d", d->low);

            UTILS_Math_SplitFloat(d->rainfall, 2, &i_part, &d_part, &sign);
            lv_label_set_text_fmt(ui_LabelRainFallVal1, "%d.%02u", (int)i_part, (unsigned int)d_part);

            lv_label_set_text_fmt(ui_LabelRainProbVal1, "%d", (int)d->precip);
            lv_label_set_text(ui_LabelWindDir1, d->wind_direction);
            lv_label_set_text_fmt(ui_LabelWindAngle1, "%d", d->wind_direction_degree);

            UTILS_Math_SplitFloat(d->wind_speed, 1, &i_part, &d_part, &sign);
            lv_label_set_text_fmt(ui_LabelWindSpeedVal1, "%d.%u", (int)i_part, (unsigned int)d_part);

            lv_label_set_text_fmt(ui_LabelWindLevelVal1, "%d", d->wind_scale);
            lv_label_set_text_fmt(ui_LabelHumVal1, "%d", d->humidity);
        }
        else if (i == 2)
        {
            /* 第三天 */
            ui_update_weather_info(ui_ImageWeather2, ui_LabelWeatherDesc2, d->code_day);

            lv_label_set_text_fmt(ui_LabelTempMaxNUM2, "%d", d->high);
            lv_label_set_text_fmt(ui_LabelTempMinNUM2, "%d", d->low);

            UTILS_Math_SplitFloat(d->rainfall, 2, &i_part, &d_part, &sign);
            lv_label_set_text_fmt(ui_LabelRainFallVal2, "%d.%02u", (int)i_part, (unsigned int)d_part);

            lv_label_set_text_fmt(ui_LabelRainProbVal2, "%d", (int)d->precip);
            lv_label_set_text(ui_LabelWindDir2, d->wind_direction);
            lv_label_set_text_fmt(ui_LabelWindAngle2, "%d", d->wind_direction_degree);

            UTILS_Math_SplitFloat(d->wind_speed, 1, &i_part, &d_part, &sign);
            lv_label_set_text_fmt(ui_LabelWindSpeedVal2, "%d.%u", (int)i_part, (unsigned int)d_part);

            lv_label_set_text_fmt(ui_LabelWindLevelVal2, "%d", d->wind_scale);
            lv_label_set_text_fmt(ui_LabelHumVal2, "%d", d->humidity);
        }
    }
}

/**
 * @brief  同时更新图标和描述文字
 * @param  img_obj: 图标对象 (如 ui_ImageWeather)
 * @param  label_obj: 描述文字对象 (如 ui_LabelWeatherDesc)
 * @param  code: 天气代码
 */
void ui_update_weather_info(lv_obj_t *img_obj, lv_obj_t *label_obj, int code)
{
    if (!img_obj || !label_obj)
        return;

    for (int i = 0; i < WEATHER_MASTER_COUNT; i++)
    {
        if (weather_master_map[i].code == code)
        {
            // 找到匹配项，直接设置图片和文字
            lv_image_set_src(img_obj, weather_master_map[i].img_src);
            lv_label_set_text(label_obj, weather_master_map[i].desc);
            return;
        }
    }

    // 如果没找到，显示“未知”
    lv_image_set_src(img_obj, &ui_img_weather_code99_unknown_png);
    lv_label_set_text(label_obj, "未知");
}
