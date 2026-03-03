/**
 * @file    bsp_bh1750.h
 * @brief   bh1750 光照强度驱动模块
 */

#ifndef __BSP_BH1750_H
#define __BSP_BH1750_H

#include <stdint.h>
#include "pal_i2c_interface.h"

#define BH1750_I2C_SLAVE_ADDRESS 0x46 /* BH1750 I2C 从机地址 */

/* 操作码定义 */
#define BH1750_POWER_DOWN 0x00  /* 进入掉电模式。芯片上电后缺省就是 Power Down 模式 */
#define BH1750_POWER_ON 0x01    /* 上电，等待测量命令 */
#define BH1750_RESET 0x07       /* 清零数据寄存器 (Power Down 模式无效) */
#define BH1750_CON_H_RES 0x10   /* 连续高分辨率模式1, 1lx 分辨率, 测量时间 120ms（最大 180ms） */
#define BH1750_CON_H_RES2 0x11  /* 连续高分辨率模式2, 0.5lx 分辨率, 测量时间 120ms */
#define BH1750_CON_L_RES 0x13   /* 连续低分辨率模式, 4lx 分辨率, 测量时间 16ms */
#define BH1750_ONE_H_RES 0x20,  /* 单次高分辨率模式 , 之后自动进入 Power Down */
#define BH1750_ONE_H_RES2 0x21, /* 单次高分辨率模式2 , 之后自动进入 Power Down  */
#define BH1750_ONE_L_RES 0x23,  /* 单次低分辨率模式 , 之后自动进入 Power Down  */

/**
 * @brief  BH1750 数据结构体
 */
typedef struct
{
    float lux; /* 转换后的光照强度 [Lux] */
} bh1750_data_t;

/**
 * @brief  BH1750 当前的状态和属性结构体
 */
typedef struct
{
    pal_i2c_interface_t interface; /* I2C 硬件接口适配层 */
    uint8_t sensitivity;           /* 灵敏度 */
    uint8_t mode;                  /* 测量模式 */
    uint8_t dev_address;           /* 设备的 I2C 从机地址 */
    bh1750_data_t data;            /* 光照强度数据 */
} bh1750_handle_t;

/* 函数声明 */
uint8_t BSP_BH1750_Init(pal_i2c_interface_t *intf);
void BSP_BH1750_ChageMode(uint8_t _ucMode);
void BSP_BH1750_AdjustSensitivity(uint8_t _ucMTReg);
uint8_t BSP_BH1750_UpdateData(void);

const bh1750_data_t *BSP_BH1750_GetData(void);

#endif
