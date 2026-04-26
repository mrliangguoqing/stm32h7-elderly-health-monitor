/**
 * @file    bsp_ws2812b.c
 * @brief   WS2812B 驱动模块
 */

#include "bsp_ws2812b.h"
#include "spi.h"

#include "bsp_delay.h"
#include "bsp_ws2812b.h"
#include "pal_log.h"

#define WS2812_SPI_Handle hspi1

ws2812b_rgb_color_t ws2812b_led_data[WS2812B_QUANTITY] = {0};

/* 强制定位到 AXI SRAM 中间位置 (0x24020000)，避开起始地址冲突，即使 .sct 被 Keil 还原也能正常工作 */
__ALIGNED(32)
uint8_t spi_dma_buffer[SPI_DMA_BUFFER_SIZE] __attribute__((section(".bss.ARM.__at_0x24020000")));

/**
 * @brief 设置指定灯珠的 RGB 颜色分量
 * @param  index: 灯珠的索引，范围 0~WS2812B_QUANTITY-1
 * @param  r: 红色亮度值 (0 - 255)
 * @param  g: 绿色亮度值 (0 - 255)
 * @param  b: 蓝色亮度值 (0 - 255)
 */
void BSP_WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index < WS2812B_QUANTITY)
    {
        ws2812b_led_data[index].r = r;
        ws2812b_led_data[index].g = g;
        ws2812b_led_data[index].b = b;
    }
}

/**
 * @brief HSV 转 RGB
 * @param  index: 灯珠的索引，范围 0~WS2812B_QUANTITY-1
 * @param  h: 色调 (0.0 - 360.0)
 * @param  s: 饱和度 (0.0 - 1.0)
 * @param  v: 亮度 (0.0 - 1.0)
 */
void BSP_WS2812_SetHSV(uint16_t index, float h, float s, float v)
{
    float r = 0.0f, g = 0.0f, b = 0.0f;
    int i = (int)(h / 60.0f) % 6;
    float f = (h / 60.0f) - (int)(h / 60.0f);
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i)
    {
    case 0:
        r = v, g = t, b = p;
        break;
    case 1:
        r = q, g = v, b = p;
        break;
    case 2:
        r = p, g = v, b = t;
        break;
    case 3:
        r = p, g = q, b = v;
        break;
    case 4:
        r = t, g = p, b = v;
        break;
    case 5:
        r = v, g = p, b = q;
        break;
    }
    /* 转换为 0-255 并写入 */
    BSP_WS2812_SetPixel(index, (uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

/**
 * @brief 将 RGB 数据根据协议转换后使用 SPI DMA 发送
 */
void BSP_WS2812_Update(void)
{

    /* 等待上一次传输完成 */
    if (WS2812_SPI_Handle.State == HAL_SPI_STATE_BUSY_TX)
    {
        return;
    }

    uint32_t dat;
    uint32_t pos = 0;

    for (int i = 0; i < WS2812B_QUANTITY; i++)
    {
        /* WS2812 协议顺序是 g-r-b */
        dat = (ws2812b_led_data[i].g << 16) | (ws2812b_led_data[i].r << 8) | (ws2812b_led_data[i].b);

        /* 协议要求先发送颜色分量的高位 */
        for (int bit = 23; bit >= 0; bit--)
        {
            if ((dat >> bit) & 0x01)
            {
                spi_dma_buffer[pos++] = WS2812_1;
            }
            else
            {
                spi_dma_buffer[pos++] = WS2812_0;
            }
        }
    }

    /* 末尾填充复位低电平信号 */
    for (int i = 0; i < WS2812_RESET_LEN; i++)
    {
        spi_dma_buffer[pos++] = 0x00;
    }

    // PAL_LOG(PAL_LOG_LEVEL_DEBUG, "Buffer Address: 0x%08X", (uint32_t)spi_dma_buffer);

    SCB_CleanDCache_by_Addr((uint32_t *)spi_dma_buffer, SPI_DMA_BUFFER_SIZE);
    if (HAL_SPI_Transmit_DMA(&WS2812_SPI_Handle, spi_dma_buffer, SPI_DMA_BUFFER_SIZE) != HAL_OK)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "DMA Start Failed");
    }
}

/**
 * @brief WS2812 测试，进入后为死循环
 */
void BSP_WS2812_Test(void)
{
    uint16_t start_hue = 0;

    while (1)
    {
        for (int i = 0; i < WS2812B_QUANTITY; i++)
        {
            /* 每个灯珠的色调比前一个偏移 20 度 */
            uint16_t hue = (start_hue + (i * 20)) % 360;
            BSP_WS2812_SetHSV(i, (float)hue, 1.0f, 0.1f);
        }
        BSP_WS2812_Update();
        start_hue = (start_hue + 5) % 360; /* 偏移起始色调，产生流动感 */
        BSP_DelayMs(30);
    }
}
