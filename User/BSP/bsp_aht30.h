/**
 * @file    bsp_aht30.h
 * @brief   aht30 温湿度驱动模块
 */

#ifndef __BSP_AHT30_H
#define __BSP_AHT30_H

#include <stdint.h>
#include "pal_i2c_interface.h"

#define AHT30_I2C_SLAVE_ADDRESS 0x70 /* AHT30 I2C 从机地址 */

/**
 * @brief  AHT30 传感器测量数据结构体
 */
typedef struct
{
    float temperature; /* 转换后的温度值 (单位: ℃) */
    float humidity;    /* 转换后的相对湿度值 (单位: %RH) */
    uint8_t status;    /* 传感器返回的状态字 */
} aht30_data_t;

/**
 * @brief  AHT30 设备句柄结构体
 */
typedef struct
{
    pal_i2c_interface_t interface; /* 硬件接口适配层 */
    aht30_data_t data;           /* 存储最新的测量结果 */
    uint8_t dev_address;           /* 设备的 I2C 从机地址 */
} aht30_handle_t;

/* 函数声明 */
uint8_t BSP_AHT30_Init(void);
uint8_t BSP_AHT30_UpdateData(void);
const aht30_data_t *BSP_AHT30_GetData(void);

#endif
