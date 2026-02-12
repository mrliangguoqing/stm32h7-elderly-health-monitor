/**
 * @file    app_system_monitor.h
 * @brief   系统监视与诊断模块头文件
 * @details 提供任务运行状态、堆栈剩余空间及 CPU 使用率的监控接口。
 * 该模块依赖于 app_config.h 中的 APP_SYSTEM_MONITOR_ENABLE 宏开关。
 */

#ifndef __APP_SYSTEM_MONITOR_H
#define __APP_SYSTEM_MONITOR_H

#include "app_config.h"

/* 根据配置开关决定是否暴露接口 */
#if (APP_SYSTEM_MONITOR_ENABLE == 1)

void App_System_Monitor_Init(void);

#endif

#endif
