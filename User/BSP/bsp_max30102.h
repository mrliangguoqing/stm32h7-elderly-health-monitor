/**
 * @file    bsp_max30102.h
 * @brief   Max30102 心率血氧模块
 */

#ifndef __BSP_MAX30102_H
#define __BSP_MAX30102_H

#include <stdint.h>

#define MAX30102_I2C_SLAVE_ADDRESS 0xAE /* MAX30102 I2C 从机地址 */

#define MAX30102_BUFFER_LEN 500 /* 5秒数据缓冲区 */
#define UPDATE_SIZE 100         /* 每次滑动更新的数据量 */

/* MAX30102 寄存器地址定义 */
#define MAX30102_INTR_STATUS_1_REG_ADDR 0x00
#define MAX30102_INTR_STATUS_2_REG_ADDR 0x01
#define MAX30102_INTR_ENABLE_1_REG_ADDR 0x02
#define MAX30102_INTR_ENABLE_2_REG_ADDR 0x03
#define MAX30102_FIFO_WR_PTR_REG_ADDR 0x04
#define MAX30102_OVF_COUNTER_REG_ADDR 0x05
#define MAX30102_FIFO_RD_PTR_REG_ADDR 0x06
#define MAX30102_FIFO_DATA_REG_ADDR 0x07
#define MAX30102_FIFO_CONFIG_REG_ADDR 0x08
#define MAX30102_MODE_CONFIG_REG_ADDR 0x09
#define MAX30102_SPO2_CONFIG_REG_ADDR 0x0A
#define MAX30102_LED1_PA_REG_ADDR 0x0C
#define MAX30102_LED2_PA_REG_ADDR 0x0D
#define MAX30102_PILOT_PA_REG_ADDR 0x10
#define MAX30102_PART_ID_REG_ADDR 0xFF

/**
 * @brief  MAX30102 传感器原始数据结构体
 */
typedef struct
{
    uint32_t ir_buffer[MAX30102_BUFFER_LEN];  /* IR (红外光) 原始数据缓冲区 */
    uint32_t red_buffer[MAX30102_BUFFER_LEN]; /* Red (红光) 原始数据缓冲区 */
} max30102_buffer_t;

/**
 * @brief  MAX30102 传感器测量数据结构体
 */
typedef struct
{
    int32_t heart_rate;      /* 计算出的心率值 (BPM) */
    int8_t heart_rate_valid; /* 心率有效性标志 (1:有效, 0:无效) */
    int32_t spo2;            /* 计算出的血氧饱和度值 (%) */
    int8_t spo2_valid;       /* 血氧有效性标志 (1:有效, 0:无效) */
} max30102_data_t;

/**
 * @brief  MAX30102 设备句柄结构体
 */
typedef struct
{
    max30102_buffer_t buffer; /* 传感器原始采样数据缓存句柄 */
    max30102_data_t data;     /* 经算法处理后的生理参数结果 */
} max30102_handle_t;

/* 函数声明 */
void BSP_MAX30102_Reset(void);
void BSP_MAX30102_ClearInterrupt(void);
uint8_t BSP_MAX30102_Init(void);
void BSP_MAX30102_ReadFifo(uint32_t *red_led, uint32_t *ir_led);

extern max30102_handle_t max30102_handle;

#endif