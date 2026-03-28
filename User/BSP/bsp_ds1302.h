/**
 * @file    bsp_ds1302.h
 * @brief   DS1302 时钟驱动模块头文件
 */

#ifndef __BSP_DS1302_H
#define __BSP_DS1302_H

#include <stdint.h>

/* 引脚宏定义 */
#define DS1302_RST_Pin GPIO_PIN_7
#define DS1302_RST_GPIO_Port GPIOA
#define DS1302_IO_Pin GPIO_PIN_4
#define DS1302_IO_GPIO_Port GPIOC
#define DS1302_SCLK_Pin GPIO_PIN_5
#define DS1302_SCLK_GPIO_Port GPIOC

/* DS1302 寄存器写地址定义 */
#define DS1302_SEC_ADDR 0x80   /* 秒寄存器写地址 (范围: 00-59, Bit7为CH停止位) */
#define DS1302_MIN_ADDR 0x82   /* 分寄存器写地址 (范围: 00-59) */
#define DS1302_HOUR_ADDR 0x84  /* 时寄存器写地址 (范围: 01-12或00-23) */
#define DS1302_DAY_ADDR 0x86   /* 日寄存器写地址 (范围: 01-31) */
#define DS1302_MONTH_ADDR 0x88 /* 月寄存器写地址 (范围: 01-12) */
#define DS1302_WEEK_ADDR 0x8A  /* 星期寄存器写地址 (范围: 1-7, 1代表星期日) */
#define DS1302_YEAR_ADDR 0x8C  /* 年寄存器写地址 (范围: 00-99) */
#define DS1302_WP_ADDR 0x8E    /* 写保护寄存器写地址 (Bit7为WP位, 1禁止写入) */

/**
 * @brief  DS1302 数据结构体
 */
typedef struct
{
    uint16_t year;  /* 年份数据 (例如: 2026) */
    uint8_t month;  /* 月份数据 (范围: 01-12) */
    uint8_t day;    /* 日期数据 (范围: 01-31) */
    uint8_t hour;   /* 小时数据 (范围: 00-23) */
    uint8_t minute; /* 分钟数据 (范围: 00-59) */
    uint8_t second; /* 秒钟数据 (范围: 00-59) */
    uint8_t week;   /* 星期数据 (范围: 1-7, 1代表星期日) */
} ds1302_data_t;

/* 函数声明 */
void BSP_DS1302_Init(void);
void BSP_DS1302_SetTime(ds1302_data_t *time);
void BSP_DS1302_UpdateData(ds1302_data_t *time);
ds1302_data_t *BSP_DS1302_GetData(void);

#endif
