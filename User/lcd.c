#include "lcd.h"
#include "main.h"

_lcd_dev lcddev;

/**
 * @brief  设置 LCD 数据写入的地址窗口区域
 * @param  xStart: 窗口左上角 X 轴坐标
 * @param  yStart: 窗口左上角 Y 轴坐标
 * @param  xEnd:   窗口右下角 X 轴坐标
 * @param  yEnd:   窗口右下角 Y 轴坐标
 * @retval None
 */
void LCD_SetAddressWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
	LCD_WR_REG(lcddev.setxcmd);
	
	LCD_WR_DATA(xStart >> 8);
	LCD_WR_DATA(xStart & 0xFF);
	LCD_WR_DATA(xEnd >> 8);
	LCD_WR_DATA(xEnd & 0xFF);

	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(yStart >> 8);
	LCD_WR_DATA(yStart & 0xFF);
	LCD_WR_DATA(yEnd >> 8);
	LCD_WR_DATA(yEnd & 0xFF);

	LCD_WR_REG(lcddev.wramcmd);
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
void LCD_FillRGBBlock(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd, uint16_t *pPixelData)
{
	uint32_t size = (xEnd - xStart + 1) * (yEnd - yStart + 1);
	LCD_SetAddressWindow(xStart, yStart, xEnd, yEnd);

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
void LCD_Clear(uint16_t color)
{
	uint32_t total = lcddev.width * lcddev.height;
	LCD_SetAddressWindow(0, 0, lcddev.width - 1, lcddev.height - 1);

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
void LCD_SetDirection(uint8_t direction)
{
	lcddev.setxcmd = 0x2A;
	lcddev.setycmd = 0x2B;
	lcddev.wramcmd = 0x2C;

	switch (direction)
	{
	case 0:
		lcddev.width = 320;
		lcddev.height = 480;
		LCD_WR_REG(0x36);
		LCD_WR_DATA((1 << 3) | (1 << 6));
		break;
	case 1:
		lcddev.width = 480;
		lcddev.height = 320;
		LCD_WR_REG(0x36);
		LCD_WR_DATA((1 << 3) | (1 << 5));
		break;
	case 2:
		lcddev.width = 320;
		lcddev.height = 480;
		LCD_WR_REG(0x36);
		LCD_WR_DATA((1 << 3) | (1 << 7) | (1 << 4));
		break;
	case 3:
		lcddev.width = 480;
		lcddev.height = 320;
		LCD_WR_REG(0x36);
		LCD_WR_DATA((1 << 3) | (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4));
		break;
	}
}

/**
 * @brief  初始化 LCD 屏幕硬件及驱动 IC
 * @note   包含复位信号控制、ST7796 寄存器配置、方向设置及初始清屏
 * @param  None
 * @retval None
 */
void LCD_Init(void)
{
	// 复位操作 (根据实际引脚修改)
	HAL_GPIO_WritePin(LCD_RES_GPIO_Port, LCD_RES_Pin, GPIO_PIN_RESET);
	BSP_DWT_DelayMs(50);
	HAL_GPIO_WritePin(LCD_RES_GPIO_Port, LCD_RES_Pin, GPIO_PIN_SET);
	BSP_DWT_DelayMs(50);

	// ST7796S 核心初始化序列
	LCD_WR_REG(0xF0);
	LCD_WR_DATA(0xC3);
	LCD_WR_REG(0xF0);
	LCD_WR_DATA(0x96);
	LCD_WR_REG(0x36);
	LCD_WR_DATA(0x68);
	LCD_WR_REG(0x3A);
	LCD_WR_DATA(0x05);
	LCD_WR_REG(0xB0);
	LCD_WR_DATA(0x80);
	LCD_WR_REG(0xB6);
	LCD_WR_DATA(0x20);
	LCD_WR_DATA(0x02);
	LCD_WR_REG(0xB5);
	LCD_WR_DATA(0x02);
	LCD_WR_DATA(0x03);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x04);
	LCD_WR_REG(0xB1);
	LCD_WR_DATA(0x80);
	LCD_WR_DATA(0x10);
	LCD_WR_REG(0xB4);
	LCD_WR_DATA(0x00);
	LCD_WR_REG(0xB7);
	LCD_WR_DATA(0xC6);
	LCD_WR_REG(0xC5);
	LCD_WR_DATA(0x24);
	LCD_WR_REG(0xE4);
	LCD_WR_DATA(0x31);
	LCD_WR_REG(0xE8);
	LCD_WR_DATA(0x40);
	LCD_WR_DATA(0x8A);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x29);
	LCD_WR_DATA(0x19);
	LCD_WR_DATA(0xA5);
	LCD_WR_DATA(0x33);

	// Gamma 设置
	LCD_WR_REG(0xE0);
	LCD_WR_DATA(0xF0);
	LCD_WR_DATA(0x09);
	LCD_WR_DATA(0x13);
	LCD_WR_DATA(0x12);
	LCD_WR_DATA(0x12);
	LCD_WR_DATA(0x2B);
	LCD_WR_DATA(0x3C);
	LCD_WR_DATA(0x44);
	LCD_WR_DATA(0x4B);
	LCD_WR_DATA(0x1B);
	LCD_WR_DATA(0x18);
	LCD_WR_DATA(0x17);
	LCD_WR_DATA(0x1D);
	LCD_WR_DATA(0x21);
	LCD_WR_REG(0XE1);
	LCD_WR_DATA(0xF0);
	LCD_WR_DATA(0x09);
	LCD_WR_DATA(0x13);
	LCD_WR_DATA(0x0C);
	LCD_WR_DATA(0x0D);
	LCD_WR_DATA(0x27);
	LCD_WR_DATA(0x3B);
	LCD_WR_DATA(0x44);
	LCD_WR_DATA(0x4D);
	LCD_WR_DATA(0x0B);
	LCD_WR_DATA(0x17);
	LCD_WR_DATA(0x17);
	LCD_WR_DATA(0x1D);
	LCD_WR_DATA(0x21);

	LCD_WR_REG(0X36);
	LCD_WR_DATA(0xEC);
	LCD_WR_REG(0xF0);
	LCD_WR_DATA(0xC3);
	LCD_WR_REG(0xF0);
	LCD_WR_DATA(0x69);
	LCD_WR_REG(0X13);
	LCD_WR_REG(0X11);
	HAL_Delay(120);
	LCD_WR_REG(0X29);

	LCD_SetDirection(3); // 默认横屏
	LCD_Clear(WHITE);
}