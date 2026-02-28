#include "bsp_gt911.h"
#include "i2c.h"
#include "main.h"
#include "bsp_lcd.h"
#include "debug.h"

/* LCD 和 GT911 共用复位引脚 */
#define GT911_RST_Pin LCD_RES_Pin
#define GT911_RST_GPIO_Port LCD_RES_GPIO_Port

extern I2C_HandleTypeDef hi2c2;

_gt911_dev gt911dev = {{0}, {0}, 0};

/* 定义 GT911 触摸点的坐标地址数组 */
const uint16_t gt911_touch_point_addr[] = {0x8150, 0x8158, 0x8160, 0x8168, 0x8170};

/**
 * @brief  GI911 I2C 底层写函数
 * @param  addr: 写寄存器的地址
 * @param  data: 待发送的数据
 * @retval 0-成功, 1-失败
 */
static uint8_t GT911_WriteData(uint16_t addr, uint8_t data)
{
	return (HAL_I2C_Mem_Write(&hi2c2, GT911_IIC_WRITE_ADDR, addr, I2C_MEMADD_SIZE_16BIT, &data, 1, 100) == HAL_OK) ? 0 : 1;
}

/**
 * @brief  GI911 I2C 底层读函数
 * @param  addr: 读寄存器的地址
 * @param  data: 接收数据存储缓冲区首地址
 * @param  len:  待接收数据长度
 * @retval 0-成功, 1-失败
 */
static uint8_t GT911_ReadData(uint16_t addr, uint8_t *data, uint8_t len)
{
	return (HAL_I2C_Mem_Read(&hi2c2, GT911_IIC_READ_ADDR, addr, I2C_MEMADD_SIZE_16BIT, data, len, 100) == HAL_OK) ? 0 : 1;
}

/**
 * @brief  GT911 传感器初始化
 * @note   因 LCD 和 GT911 共用复位引脚，在此函数中统一执行复位操作（先调用 BSP_GT911_Init ，再调用 LCD_Init）
 * @param  None
 * @retval 0-成功, 其他-失败
 */
uint8_t BSP_GT911_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* 把 RST 和 INT 都设为推挽输出 */
	GPIO_InitStruct.Pin = GT911_RST_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GT911_RST_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GT911_INT_Pin;
	HAL_GPIO_Init(GT911_INT_GPIO_Port, &GPIO_InitStruct);

	/* 开始设置地址 */
	HAL_GPIO_WritePin(GT911_RST_GPIO_Port, GT911_RST_Pin, GPIO_PIN_RESET); /* RST = 0 */
	HAL_GPIO_WritePin(GT911_INT_GPIO_Port, GT911_INT_Pin, GPIO_PIN_RESET); /* INT = 0 */
	BSP_DWT_DelayMs(50);												   /* 满足 LCD 的复位时间要求，无 LCD 时延时大于 10ms 即可 */

	/* 根据 GT911 的 I2C 从设备地址设定 INT 电平 */
#if (GT911_IIC_WRITE_ADDR == 0x28 && GT911_IIC_READ_ADDR == 0x29)
	HAL_GPIO_WritePin(GT911_INT_GPIO_Port, GT911_INT_Pin, GPIO_PIN_SET); /* 0x28/0x29，INT 为高电平 */
#else
	HAL_GPIO_WritePin(GT911_INT_GPIO_Port, GT911_INT_Pin, GPIO_PIN_RESET); /* 0xBA/0xBB，INT 为低电平 */
#endif
	BSP_DWT_DelayMs(1); /* 延时大于 100us */

	HAL_GPIO_WritePin(GT911_RST_GPIO_Port, GT911_RST_Pin, GPIO_PIN_SET); /* RST = 1 */
	BSP_DWT_DelayMs(10);												 /* 延时大于 5ms */

	/* 将 INT 切换为浮空输入 */
	HAL_GPIO_DeInit(GT911_INT_GPIO_Port, GT911_INT_Pin);
	GPIO_InitStruct.Pin = GT911_INT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GT911_INT_GPIO_Port, &GPIO_InitStruct);

	BSP_DWT_DelayMs(100); /* 等待芯片初始化完成 */

	return BSP_GT911_ReadId();
}

/**
 * @brief  GT911 读取产品 ID
 * @note   产品 ID 为 911
 * @param  None
 * @retval 0-成功, 其他-失败
 */
uint8_t BSP_GT911_ReadId(void)
{
	uint8_t id[4];

	/* 读取产品 ID */
	if (GT911_ReadData(GT911_PRODUCT_ID_REG_ADDR, id, 4) == 0)
	{
		if (id[0] == '9')
		{
			return 0;
		}
	}
	return 1;
}

/**
 * @brief  GT911 扫描函数
 * @note   读取触摸数据，将数据更新至结构体
 * @param  None
 * @retval 0-成功, 其他-失败
 */
uint8_t BSP_GT911_Scan(void)
{
	uint8_t buf[4], status, touch_number;

	/* 从 GT911 读取触摸状态寄存器，获取状态信息 */
	if (GT911_ReadData(GT911_TOUCH_STATUS_REG_ADDR, &status, 1) != 0)
	{
		return 1; /* 读取失败 */
	}

	/* 检查触摸状态的高位是否为 1，表示有触摸 */
	if (status & 0x80)
	{
		/* 获取触摸点的数量，状态的低4位表示触摸点的数量 */
		touch_number = status & 0x0F;

		uint8_t clean = 0;
		/* 清除触摸状态寄存器，防止数据溢出或错误 */
		GT911_WriteData(GT911_TOUCH_STATUS_REG_ADDR, clean);

		if (touch_number <= GT911_TOUCH_NUMBER)
		{
			gt911dev.sta = 0x80; /* 设置触摸屏状态为触摸有效 */

			/* 遍历所有触摸点，读取并处理坐标数据 */
			for (uint8_t i = 0; i < touch_number; i++)
			{
				/* 从指定的触摸点坐标地址读取数据（每个触摸点4字节） */
				if (GT911_ReadData(gt911_touch_point_addr[i], buf, 4) == 0)
				{
					/* 获取原始的触摸点坐标数据 */
					uint16_t temp_y = ((uint16_t)buf[1] << 8) + buf[0];
					uint16_t temp_x = ((uint16_t)buf[3] << 8) + buf[2];

					/* 坐标变换：交换 X 和 Y 坐标，且翻转 X 坐标 */
					gt911dev.x[i] = lcddev.width - temp_x;
					gt911dev.y[i] = temp_y;

					/* 设置当前触摸点状态为有效 */
					gt911dev.sta |= (1 << i);
				}
			}
			return 0; /* 成功获取触摸数据 */
		}
	}

	/* 如果状态寄存器中的触摸标志位无效，或触摸点数为 0 */
	if ((status & 0x8F) == 0x80 || (status & 0x0F) == 0)
	{
		gt911dev.sta = 0; /* 清除触摸状态 */
	}

	return 1; /* 没有触摸点或者读取失败 */
}

/**
 * @brief  GT911 触摸测试函数
 * @note   仅开发测试使用，进入函数后为死循环，将触摸坐标通过串口输出打印
 * @param  None
 * @retval None
 */
void BSP_GT911_Test(void)
{
	uint8_t last_sta = 0; /* 用于记录上一次的触摸状态 */

	printf("\r\n--- GT911 Monitor ---\r\n");

	for (;;)
	{
		BSP_GT911_Scan();

		/* 当前有按下 (sta 的 bit7 为 1) */
		if (gt911dev.sta & 0x80)
		{
			uint8_t touch_number = 0;
			for (uint8_t i = 0; i < GT911_TOUCH_NUMBER; i++)
			{
				if (gt911dev.sta & (1 << i))
					touch_number++;
			}

			printf("TOUCH DOWN | Points: %d | ", touch_number);
			for (uint8_t i = 0; i < 5; i++)
			{
				if (gt911dev.sta & (1 << i))
				{
					printf("P%d:[%3d,%3d] ", i, gt911dev.x[i], gt911dev.y[i]);
				}
			}
			printf("\r\n");
			last_sta = 1; /* 标记当前处于按下状态 */
		}
		/* 刚抬起瞬间 (上次是按下，这次 sta 为 0) */
		else if (last_sta == 1)
		{
			printf("TOUCH UP | All points released.\r\n");
			last_sta = 0; /* 标记当前已释放 */
		}

		BSP_DWT_DelayMs(10); /* 间隔 10ms 采样 */
	}
}
