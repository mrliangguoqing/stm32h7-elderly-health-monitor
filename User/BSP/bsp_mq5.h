/**
 * @file    bsp_mq5.h
 * @brief   MQ-5 可燃气体传感器驱动模块
 */

#ifndef __BSP_MQ5_H
#define __BSP_MQ5_H

#include <stdint.h>

#define MQ5_FILTER_SIZE 20 /* 滤波窗口大小，决定了平滑度和响应延迟 */
#define MQ5_DIVIDER 1.5f   /* 硬件分压电路系数，用于将引脚电压还原为传感器输出电压 */

/**
 * @brief MQ-5 传感器数据结构体
 */
typedef struct
{
    uint16_t raw_data; /* ADC 原始采样值 (0-65535) */
    float voltage;     /* 还原后的即时电压 (单位: V) */
    float smooth_v;    /* 经过滑动平均滤波后的电压 (单位: V) */
    float ppm;         /* 计算出的气体浓度值 (待扩展) */

    uint16_t filter_buf[MQ5_FILTER_SIZE]; /* 滤波滑动窗口缓冲区 */
    uint8_t buf_index;                    /* 缓冲区当前索引指针 */
} mq5_data_t;

/* 函数声明 */
void BSP_MQ5_Init(void);
void BSP_MQ5_UpdateRaw(uint16_t raw); // 中断调用
void BSP_MQ5_Process(void);           // 任务调用
const mq5_data_t *BSP_MQ5_GetData(void);

#endif
