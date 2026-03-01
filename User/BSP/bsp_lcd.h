/**
 * @file    bsp_lcd.h
 * @brief   LCD 驱动模块
 */

#ifndef __BSP_LCD_H
#define __BSP_LCD_H

#include <stdint.h>
#include "stm32h7xx.h"

/* --- FMC 地址映射优化 --- */
/* 使用 Bank1 Sector1 (0x60000000)，RS(DC) 接 A16
 * H7 FMC 在16位模式下内部地址右移一位，因此硬件 A16 对应软件地址第 17 位
 */
#define LCD_ADDR_BASE ((uint32_t)0x60000000)	/* FMC Bank1 分区起始基地址 */
#define LCD_REG_OFFSET (0x00000000)/* 寄存器偏移：A16 为低电平 (写命令) */
#define LCD_DATA_OFFSET (1 << (16 + 1)) /* 数据偏移：A16 为高电平 (写数据/参数) */

/** * 定义指向 FMC 地址空间的 16 位指针 
 * __IO (即 volatile) 确保编译器不会优化掉连续的内存写入动作
 */
#define LCD_REG_PTR ((__IO uint16_t *)(LCD_ADDR_BASE | LCD_REG_OFFSET))
#define LCD_DATA_PTR ((__IO uint16_t *)(LCD_ADDR_BASE | LCD_DATA_OFFSET))

/* --- 核心内联函数：极致性能 --- */
/**
 * @brief  向 LCD 写入寄存器地址/指令
 * @param  reg: 16 位指令码
 * @note   通过操作 A16 低电平地址，告知 LCD 控制器当前总线上是指令
 */
static inline void BSP_LCD_WR_REG(uint16_t reg)
{
	*LCD_REG_PTR = reg;
}

/**
 * @brief  向 LCD 写入数据
 * @param  data: 16 位数据或像素颜色 (RGB565)
 * @note   通过操作 A16 高电平地址，告知 LCD 控制器当前总线上是参数或像素
 * static inline 保证了在大批量刷屏数据传输时的零函数调用开销
 */
static inline void BSP_LCD_WR_DATA(uint16_t data)
{
	*LCD_DATA_PTR = data;
}

/* ST7796 控制指令宏定义 */
#define LCD_SET_X_CMD   0x2A    /* 设置 X 轴起始/结束坐标 */
#define LCD_SET_Y_CMD   0x2B    /* 设置 Y 轴起始/结束坐标 */
#define LCD_WRITE_RAM   0x2C    /* 写显存指令 */

/* 颜色宏定义 (RGB565) */
#define WHITE 0xFFFF
#define BLACK 0x0000
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F

/**
 * @brief  LCD 分辨率结构体
 */
typedef struct
{
	uint16_t width;	  /* LCD 宽度 */
	uint16_t height;  /* LCD 高度 */
} lcd_res_t;

/* 函数声明 */
const lcd_res_t* BSP_LCD_GetRes(void);

void BSP_LCD_Init(void);
void BSP_LCD_Clear(uint16_t color);
void BSP_LCD_FillRGBBlock(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd, uint16_t *pPixelData);

#endif
