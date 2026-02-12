#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>
#include "stm32h7xx.h"

/* --- FMC 地址映射优化 --- */
/* 使用 Bank1 Sector1 (0x60000000)，RS(DC) 接 A16
 * H7 FMC 在16位模式下内部地址右移一位，因此硬件 A16 对应软件地址第 17 位
 */
#define LCD_ADDR_BASE ((uint32_t)0x60000000)
#define LCD_REG_OFFSET (0x00000000)
#define LCD_DATA_OFFSET (1 << (16 + 1)) // A16 偏移

#define LCD_REG_PTR ((__IO uint16_t *)(LCD_ADDR_BASE | LCD_REG_OFFSET))
#define LCD_DATA_PTR ((__IO uint16_t *)(LCD_ADDR_BASE | LCD_DATA_OFFSET))

/* --- 核心内联函数：极致性能 --- */
static inline void LCD_WR_REG(uint16_t reg)
{
	*LCD_REG_PTR = reg;
}

static inline void LCD_WR_DATA(uint16_t data)
{
	*LCD_DATA_PTR = data;
}

/* --- LCD 参数结构体 --- */
typedef struct
{
	uint16_t width;
	uint16_t height;
	uint16_t wramcmd;
	uint16_t setxcmd;
	uint16_t setycmd;
} _lcd_dev;

extern _lcd_dev lcddev;

/* --- 颜色宏定义 (RGB565) --- */
#define WHITE 0xFFFF
#define BLACK 0x0000
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F

/* --- 接口函数声明 --- */
void LCD_Init(void);
void LCD_Clear(uint16_t color);
void LCD_SetDirection(uint8_t direction);
void LCD_SetAddressWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd);
void LCD_FillRGBBlock(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd, uint16_t *pPixelData);

#endif
