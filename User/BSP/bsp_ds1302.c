/**
 * @file    bsp_ds1302.c
 * @brief   DS1302 时钟驱动模块
 */

#include "bsp_ds1302.h"
#include "bsp_dwt.h"

#include "gpio.h"

/* 内部静态实例 */
ds1302_data_t ds1302_dev = {
	.year = 2026, /* 设置初始年份 */
	.month = 3,	  /* 设置初始月份 */
	.day = 28,	  /* 设置初始日期 */
	.hour = 21,	  /* 设置初始小时 */
	.minute = 45, /* 设置初始分钟 */
	.second = 0,  /* 设置初始秒钟 */
	.week = 6	  /* 设置初始星期 */
};

/* 内部函数 */
static uint8_t BCD_To_Dec(uint8_t bcd);
static uint8_t Dec_To_BCD(uint8_t dec);
static void BSP_DS1302_IO_Mode(uint8_t mode);
static void BSP_DS1302_WriteByte(uint8_t data);
static uint8_t BSP_DS1302_ReadByte(void);
static void BSP_DS1302_WriteReg(uint8_t addr, uint8_t data);
static uint8_t BSP_DS1302_ReadReg(uint8_t addr);

/**
 * @brief  DS1302 硬件初始化及默认时间设置
 * @note   初始化引脚电平，解除写保护，并同步初始时间至芯片
 */
void BSP_DS1302_Init(void)
{
	HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DS1302_SCLK_GPIO_Port, DS1302_SCLK_Pin, GPIO_PIN_RESET);

	BSP_DS1302_WriteReg(DS1302_WP_ADDR, 0x00); /* 关闭写保护 */
	BSP_DS1302_SetTime(&ds1302_dev);		   /* 设置日期和时间 */
}

/**
 * @brief  向 DS1302 写入完整的日期和时间数据
 * @param  time: 指向包含待设置时间信息（年、月、日、时、分、秒、星期）的结构体指针
 * @note   写入前后会自动处理 WP (Write Protect) 写保护位的开关
 */
void BSP_DS1302_SetTime(ds1302_data_t *time)
{
	BSP_DS1302_WriteReg(DS1302_WP_ADDR, 0x00);							 /* 解除写保护，允许写入数据 */
	BSP_DS1302_WriteReg(DS1302_SEC_ADDR, Dec_To_BCD(time->second));		 /* 设置秒 (转换成BCD码写入) */
	BSP_DS1302_WriteReg(DS1302_MIN_ADDR, Dec_To_BCD(time->minute));		 /* 设置分 (转换成BCD码写入) */
	BSP_DS1302_WriteReg(DS1302_HOUR_ADDR, Dec_To_BCD(time->hour));		 /* 设置时 (转换成BCD码写入) */
	BSP_DS1302_WriteReg(DS1302_DAY_ADDR, Dec_To_BCD(time->day));		 /* 设置日 (转换成BCD码写入) */
	BSP_DS1302_WriteReg(DS1302_MONTH_ADDR, Dec_To_BCD(time->month));	 /* 设置月 (转换成BCD码写入) */
	BSP_DS1302_WriteReg(DS1302_WEEK_ADDR, Dec_To_BCD(time->week));		 /* 设置星期 (转换成BCD码写入) */
	BSP_DS1302_WriteReg(DS1302_YEAR_ADDR, Dec_To_BCD(time->year % 100)); /* 设置年 (仅取后两位写入) */
	BSP_DS1302_WriteReg(DS1302_WP_ADDR, 0x80);							 /* 开启写保护，防止数据被误改 */
}

/**
 * @brief  从 DS1302 读取当前日期和时间并更新到结构体
 * @param  time: 指向用于存储读取结果（十进制格式）的结构体指针
 * @note   自动处理 BCD 码至十进制的转换，并将年份补全为 20xx 格式
 */
void BSP_DS1302_UpdateData(ds1302_data_t *time)
{
	time->second = BCD_To_Dec(BSP_DS1302_ReadReg(DS1302_SEC_ADDR));		  /* 读取秒并转换为十进制 */
	time->minute = BCD_To_Dec(BSP_DS1302_ReadReg(DS1302_MIN_ADDR));		  /* 读取分并转换为十进制 */
	time->hour = BCD_To_Dec(BSP_DS1302_ReadReg(DS1302_HOUR_ADDR));		  /* 读取时并转换为十进制 */
	time->day = BCD_To_Dec(BSP_DS1302_ReadReg(DS1302_DAY_ADDR));		  /* 读取日并转换为十进制 */
	time->month = BCD_To_Dec(BSP_DS1302_ReadReg(DS1302_MONTH_ADDR));	  /* 读取月并转换为十进制 */
	time->week = BCD_To_Dec(BSP_DS1302_ReadReg(DS1302_WEEK_ADDR));		  /* 读取星期并转换为十进制 */
	time->year = BCD_To_Dec(BSP_DS1302_ReadReg(DS1302_YEAR_ADDR)) + 2000; /* 读取年并加上世纪偏移 */
}

/**
 * @brief  获取 DS1302 的数据
 * @retval 指向 DS1302 数据结构体的指针
 */
ds1302_data_t *BSP_DS1302_GetData(void)
{
	return &ds1302_dev;
}

/**
 * @brief  BCD码转十进制
 * @param  bcd: 输入的 8 位 BCD 码字节
 * @retval 转换后的十进制数值
 */
static uint8_t BCD_To_Dec(uint8_t bcd)
{
	return (uint8_t)((bcd >> 4) * 10 + (bcd & 0x0F));
}

/**
 * @brief  十进制转BCD码
 * @param  dec: 输入的十进制数值
 * @retval 转换后的 8 位 BCD 码字节
 */
static uint8_t Dec_To_BCD(uint8_t dec)
{
	return (uint8_t)(((dec / 10) << 4) | (dec % 10));
}

/**
 * @brief  配置 DS1302 的双向 IO 引脚方向
 * @param  mode: 0-切换为输入模式 (准备读取数据), 1-切换为输出模式 (准备发送数据)
 * @note   输入模式下使用内部上拉以增强信号稳定性
 */
static void BSP_DS1302_IO_Mode(uint8_t mode)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = DS1302_IO_Pin;

	if (mode == 0)
	{
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT; /* 输入 */
		GPIO_InitStruct.Pull = GPIO_PULLUP;		/* 内部上拉+外部上拉 */
	}
	else if (mode == 1)
	{
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; /* 推挽输出 */
		GPIO_InitStruct.Pull = GPIO_NOPULL;
	}

	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

	HAL_GPIO_Init(DS1302_IO_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief  向 DS1302 写入一个字节数据
 * @param  data: 待写入的 8 位数据 (从低位 LSB 开始发送)
 * @note   写入过程中会自动将 IO 引脚切换为输出模式
 */
static void BSP_DS1302_WriteByte(uint8_t data)
{
	BSP_DS1302_IO_Mode(1); /* 设为输出模式 */
	for (uint8_t i = 0; i < 8; i++)
	{
		HAL_GPIO_WritePin(DS1302_SCLK_GPIO_Port, DS1302_SCLK_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DS1302_IO_GPIO_Port, DS1302_IO_Pin, (data & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
		data >>= 1;
		BSP_DWT_DelayUs(2);
		HAL_GPIO_WritePin(DS1302_SCLK_GPIO_Port, DS1302_SCLK_Pin, GPIO_PIN_SET);
		BSP_DWT_DelayUs(2);
	}
}

/**
 * @brief  从 DS1302 读取一个字节数据
 * @retval 读取到的 8 位数据 (从低位 LSB 开始接收)
 * @note   读取前会自动将 IO 引脚切换为输入模式，以释放总线控制权
 */
static uint8_t BSP_DS1302_ReadByte(void)
{
	uint8_t data = 0;
	BSP_DS1302_IO_Mode(0); /* 设为输入模式 */
	for (uint8_t i = 0; i < 8; i++)
	{
		HAL_GPIO_WritePin(DS1302_SCLK_GPIO_Port, DS1302_SCLK_Pin, GPIO_PIN_RESET);
		BSP_DWT_DelayUs(2);
		if (HAL_GPIO_ReadPin(DS1302_IO_GPIO_Port, DS1302_IO_Pin) == GPIO_PIN_SET)
		{
			data |= (0x01 << i);
		}
		HAL_GPIO_WritePin(DS1302_SCLK_GPIO_Port, DS1302_SCLK_Pin, GPIO_PIN_SET);
		BSP_DWT_DelayUs(2);
	}
	return data;
}

/**
 * @brief  向 DS1302 指定寄存器写入数据
 * @param  addr: 寄存器写地址
 * @param  data: 待写入的 8 位数据
 */
static void BSP_DS1302_WriteReg(uint8_t addr, uint8_t data)
{
	HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DS1302_SCLK_GPIO_Port, DS1302_SCLK_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_SET); // 启动通信
	BSP_DWT_DelayUs(2);

	BSP_DS1302_WriteByte(addr); // 发送命令/地址
	BSP_DS1302_WriteByte(data); // 发送数据

	HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_RESET); // 结束通信
}

/**
 * @brief  从 DS1302 指定寄存器读取数据
 * @param  addr: 寄存器读地址 (若传入写地址，函数会自动将其转换为读地址)
 * @retval 读取到的 8 位寄存器数据
 */
static uint8_t BSP_DS1302_ReadReg(uint8_t addr)
{
	uint8_t data = 0;
	HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DS1302_SCLK_GPIO_Port, DS1302_SCLK_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_SET);
	BSP_DWT_DelayUs(2);

	BSP_DS1302_WriteByte(addr | 0x01); // 确保是读地址
	data = BSP_DS1302_ReadByte();

	HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_RESET);
	return data;
}
