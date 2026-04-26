/**
 * @file    bsp_ws2812b.h
 * @brief   WS2812B 驱动模块头文件
 */

#ifndef __BSP_WS2812B_H
#define __BSP_WS2812B_H

#include <stdint.h>

#define WS2812_0 0xC0        /* 11000000 (高电平 320ns) */
#define WS2812_1 0xF8        /* 11111000 (高电平 800ns) */
#define WS2812_RESET_LEN 200 /* 复位信号字节数 */

#define WS2812B_QUANTITY 4                                                                  /* 灯珠数量 */
#define SPI_DMA_BUFFER_SIZE ((((WS2812B_QUANTITY * 24 + WS2812_RESET_LEN) + 31) / 32) * 32) /* SPI DMA 发送缓冲区大小，对齐到 32 字节 */

/**
 * @brief  WS2812B RGB 颜色数据
 */
typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} ws2812b_rgb_color_t;

void BSP_WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void BSP_WS2812_SetHSV(uint16_t index, float h, float s, float v);
void BSP_WS2812_Update(void);
void BSP_WS2812_Test(void);

#endif
