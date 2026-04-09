/**
 * @file    app_rtc_alarm.c
 * @brief   应用层实时时钟、闹钟逻辑模块头文件
 */

#ifndef __APP_RTC_ALARM_H
#define __APP_RTC_ALARM_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  闹钟数据结构体
 */
typedef struct
{
    uint8_t hour;           /* 闹钟设定小时 */
    uint8_t minute;         /* 闹钟设定分钟 */
    uint8_t last_alarm_min; /* 记录上一次响铃的分钟 */
    bool is_enabled;        /* 闹钟开关状态 */
    bool is_active;         /* 闹钟是否正在触发（用于逻辑自锁） */
} alarm_config_t;

void App_RTC_Alarm_Init(void);

extern alarm_config_t g_user_alarm;

#endif
