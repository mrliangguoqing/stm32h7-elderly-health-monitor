/**
 * @file    bsp_lcd.c
 * @brief   LCD 驱动模块
 */

#include "bsp_lcd.h"
#include "tim.h"
#include "bsp_dwt.h"

static lcd_res_t lcd_res;

/**
 * @brief  获取 LCD 分辨率结构体的只读指针
 * @note   通过返回 const 指针，允许外部高效读取 LCD 的分辨率，同时防止外部直接修改私有变量 lcd_res
 * @retval 指向 LCD 资源结构体的 const 指针 (只读地址)
 */
const lcd_res_t *BSP_LCD_GetRes(void)
{
	return &lcd_res; // 返回内部私有静态变量的地址
}

/**
 * @brief  设置 LCD 背光亮度（通过 PWM 占空比调节）
 * @param  brightness: 亮度百分比，取值范围 0 到 100
 * @note   通过修改 TIM1 Channel 3 的比较寄存器（CCR）来改变占空比
 * 		   定时器的自动重装载值（ARR）配置为 999
 * @retval None
 */
static void BSP_LCD_SetBacklight(uint8_t brightness)
{
    uint32_t ccr_value = 0;

    if (brightness > 100)
    {
        brightness = 100;
    }

    /* 将百分比映射到定时器的计数空间
       ARR 为 999，总计有 1000 个刻度（0-999）
       因此将百分比乘以 10 即可得到对应的比较值。*/
    ccr_value = brightness * 10;

    if (ccr_value > 999)
    {
        ccr_value = 999;
    }

    /* 更新定时器 1 通道 3 的捕获/比较寄存器值，立即改变 PWM 占空比 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, ccr_value);
} 

/**
 * @brief  设置 LCD 数据写入的地址窗口区域
 * @param  xStart: 窗口左上角 X 轴坐标
 * @param  yStart: 窗口左上角 Y 轴坐标
 * @param  xEnd:   窗口右下角 X 轴坐标
 * @param  yEnd:   窗口右下角 Y 轴坐标
 * @retval None
 */
static void BSP_LCD_SetAddressWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
	BSP_LCD_WR_REG(LCD_SET_X_CMD);

	BSP_LCD_WR_DATA(xStart >> 8);
	BSP_LCD_WR_DATA(xStart & 0xFF);
	BSP_LCD_WR_DATA(xEnd >> 8);
	BSP_LCD_WR_DATA(xEnd & 0xFF);

	BSP_LCD_WR_REG(LCD_SET_Y_CMD);
	BSP_LCD_WR_DATA(yStart >> 8);
	BSP_LCD_WR_DATA(yStart & 0xFF);
	BSP_LCD_WR_DATA(yEnd >> 8);
	BSP_LCD_WR_DATA(yEnd & 0xFF);

	BSP_LCD_WR_REG(LCD_WRITE_RAM);
}

/**
 * @brief  向指定区域快速批量填充像素颜色数据
 * @note   利用循环展开优化 FMC 传输效率，适用于 RGB565 格式缓冲区
 * @param  xStart: 起始 X 轴坐标
 * @param  yStart: 起始 Y 轴坐标
 * @param  xEnd:   结束 X 轴坐标
 * @param  yEnd:   结束 Y 轴坐标
 * @param  pPixelData: 指向待写入颜色数据阵列的指针
 * @retval None
 */
void BSP_LCD_FillRGBBlock(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd, uint16_t *pPixelData)
{
	uint32_t size = (xEnd - xStart + 1) * (yEnd - yStart + 1);
	BSP_LCD_SetAddressWindow(xStart, yStart, xEnd, yEnd);

	uint32_t i = size / 8;
	uint32_t j = size % 8;

	while (i--)
	{
		*LCD_DATA_PTR = *pPixelData++;
		*LCD_DATA_PTR = *pPixelData++;
		*LCD_DATA_PTR = *pPixelData++;
		*LCD_DATA_PTR = *pPixelData++;
		*LCD_DATA_PTR = *pPixelData++;
		*LCD_DATA_PTR = *pPixelData++;
		*LCD_DATA_PTR = *pPixelData++;
		*LCD_DATA_PTR = *pPixelData++;
	}
	while (j--)
	{
		*LCD_DATA_PTR = *pPixelData++;
	}
}

/**
 * @brief  以单色填充整个屏幕
 * @param  color: 填充用的 RGB565 颜色值
 * @retval None
 */
void BSP_LCD_Clear(uint16_t color)
{
	uint32_t total = lcd_res.width * lcd_res.height;
	BSP_LCD_SetAddressWindow(0, 0, lcd_res.width - 1, lcd_res.height - 1);

	uint32_t i = total / 8;
	uint32_t j = total % 8;

	while (i--)
	{
		*LCD_DATA_PTR = color;
		*LCD_DATA_PTR = color;
		*LCD_DATA_PTR = color;
		*LCD_DATA_PTR = color;
		*LCD_DATA_PTR = color;
		*LCD_DATA_PTR = color;
		*LCD_DATA_PTR = color;
		*LCD_DATA_PTR = color;
	}
	while (j--)
	{
		*LCD_DATA_PTR = color;
	}
}

/**
 * @brief  设置屏幕显示方向并更新逻辑分辨率
 * @param  direction: 方向索引 (0:竖屏, 1:横屏, 2:反向竖屏, 3:反向横屏)
 * @retval None
 */
static void BSP_LCD_SetDirection(uint8_t direction)
{
	switch (direction)
	{
	case 0:
		lcd_res.width = 320;
		lcd_res.height = 480;
		BSP_LCD_WR_REG(0x36);
		BSP_LCD_WR_DATA((1 << 3) | (1 << 6));
		break;
	case 1:
		lcd_res.width = 480;
		lcd_res.height = 320;
		BSP_LCD_WR_REG(0x36);
		BSP_LCD_WR_DATA((1 << 3) | (1 << 5));
		break;
	case 2:
		lcd_res.width = 320;
		lcd_res.height = 480;
		BSP_LCD_WR_REG(0x36);
		BSP_LCD_WR_DATA((1 << 3) | (1 << 7) | (1 << 4));
		break;
	case 3:
		lcd_res.width = 480;
		lcd_res.height = 320;
		BSP_LCD_WR_REG(0x36);
		BSP_LCD_WR_DATA((1 << 3) | (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4));
		break;
	}
}

/**
 * @brief  初始化 LCD 屏幕硬件及驱动 IC
 * @note   包含复位信号控制、ST7796 寄存器配置、方向设置及初始清屏
 * @param  None
 * @retval None
 */
void BSP_LCD_Init(void)
{
	/* 复位操作，准备初始化 */
	/* 因 LCD 和 GT911 共用复位引脚，在 GT911_Init 中统一执行复位操作，故此处注释 */
	// HAL_GPIO_WritePin(LCD_RES_GPIO_Port, LCD_RES_Pin, GPIO_PIN_RESET);
	// BSP_DWT_DelayMs(50);
	// HAL_GPIO_WritePin(LCD_RES_GPIO_Port, LCD_RES_Pin, GPIO_PIN_SET);
	// BSP_DWT_DelayMs(50);

	/* ST7796S 核心初始化序列 */
	BSP_LCD_WR_REG(0xF0);
	BSP_LCD_WR_DATA(0xC3);
	BSP_LCD_WR_REG(0xF0);
	BSP_LCD_WR_DATA(0x96);
	BSP_LCD_WR_REG(0x36);
	BSP_LCD_WR_DATA(0x68);
	BSP_LCD_WR_REG(0x3A);
	BSP_LCD_WR_DATA(0x05);
	BSP_LCD_WR_REG(0xB0);
	BSP_LCD_WR_DATA(0x80);
	BSP_LCD_WR_REG(0xB6);
	BSP_LCD_WR_DATA(0x20);
	BSP_LCD_WR_DATA(0x02);
	BSP_LCD_WR_REG(0xB5);
	BSP_LCD_WR_DATA(0x02);
	BSP_LCD_WR_DATA(0x03);
	BSP_LCD_WR_DATA(0x00);
	BSP_LCD_WR_DATA(0x04);
	BSP_LCD_WR_REG(0xB1);
	BSP_LCD_WR_DATA(0x80);
	BSP_LCD_WR_DATA(0x10);
	BSP_LCD_WR_REG(0xB4);
	BSP_LCD_WR_DATA(0x00);
	BSP_LCD_WR_REG(0xB7);
	BSP_LCD_WR_DATA(0xC6);
	BSP_LCD_WR_REG(0xC5);
	BSP_LCD_WR_DATA(0x24);
	BSP_LCD_WR_REG(0xE4);
	BSP_LCD_WR_DATA(0x31);
	BSP_LCD_WR_REG(0xE8);
	BSP_LCD_WR_DATA(0x40);
	BSP_LCD_WR_DATA(0x8A);
	BSP_LCD_WR_DATA(0x00);
	BSP_LCD_WR_DATA(0x00);
	BSP_LCD_WR_DATA(0x29);
	BSP_LCD_WR_DATA(0x19);
	BSP_LCD_WR_DATA(0xA5);
	BSP_LCD_WR_DATA(0x33);

	/* Gamma 设置 */
	BSP_LCD_WR_REG(0xE0);
	BSP_LCD_WR_DATA(0xF0);
	BSP_LCD_WR_DATA(0x09);
	BSP_LCD_WR_DATA(0x13);
	BSP_LCD_WR_DATA(0x12);
	BSP_LCD_WR_DATA(0x12);
	BSP_LCD_WR_DATA(0x2B);
	BSP_LCD_WR_DATA(0x3C);
	BSP_LCD_WR_DATA(0x44);
	BSP_LCD_WR_DATA(0x4B);
	BSP_LCD_WR_DATA(0x1B);
	BSP_LCD_WR_DATA(0x18);
	BSP_LCD_WR_DATA(0x17);
	BSP_LCD_WR_DATA(0x1D);
	BSP_LCD_WR_DATA(0x21);
	BSP_LCD_WR_REG(0XE1);
	BSP_LCD_WR_DATA(0xF0);
	BSP_LCD_WR_DATA(0x09);
	BSP_LCD_WR_DATA(0x13);
	BSP_LCD_WR_DATA(0x0C);
	BSP_LCD_WR_DATA(0x0D);
	BSP_LCD_WR_DATA(0x27);
	BSP_LCD_WR_DATA(0x3B);
	BSP_LCD_WR_DATA(0x44);
	BSP_LCD_WR_DATA(0x4D);
	BSP_LCD_WR_DATA(0x0B);
	BSP_LCD_WR_DATA(0x17);
	BSP_LCD_WR_DATA(0x17);
	BSP_LCD_WR_DATA(0x1D);
	BSP_LCD_WR_DATA(0x21);

	BSP_LCD_WR_REG(0X36);
	BSP_LCD_WR_DATA(0xEC);
	BSP_LCD_WR_REG(0xF0);
	BSP_LCD_WR_DATA(0xC3);
	BSP_LCD_WR_REG(0xF0);
	BSP_LCD_WR_DATA(0x69);
	BSP_LCD_WR_REG(0X13);
	BSP_LCD_WR_REG(0X11);
	BSP_DWT_DelayMs(120);
	BSP_LCD_WR_REG(0X29);

	BSP_LCD_SetDirection(3); /* 默认横屏 */
	BSP_LCD_Clear(WHITE);
    BSP_LCD_SetBacklight(100);	/* 背光亮度 100% */
}
