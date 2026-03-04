/**
 * @file    bsp_bh1750.c
 * @brief   bh1750 光照强度驱动模块
 */

#include "bsp_bh1750.h"
#include "bsp_dwt.h"

#include "i2c.h"

/* 内部静态实例 */
static bh1750_handle_t bh1750_handle;

/**
 * @brief  BH1750 写 1 字节命令
 * @param  intf: 写入的命令
 * @retval None
 */
void BSP_BH1750_WriteCmd(uint8_t cmd)
{
	uint8_t temp_cmd = cmd; /* 将宏存入变量 */

	bh1750_handle.interface.write(bh1750_handle.dev_address, &temp_cmd, 1, bh1750_handle.interface.intf_ptr);
}

/**
 * @brief  BH1750 传感器初始化
 * @param  None
 * @retval 0-成功, 其他-失败
 */
uint8_t BSP_BH1750_Init(void)
{
	/* 配置硬件接口 */
	bh1750_handle.interface.write = PAL_STM32_I2C_Write;
	bh1750_handle.interface.read = PAL_STM32_I2C_Read;
	bh1750_handle.interface.delay = BSP_DWT_DelayMs;
	bh1750_handle.interface.intf_ptr = &hi2c1;

	/* 配置设备参数 */
	bh1750_handle.dev_address = BH1750_I2C_SLAVE_ADDRESS;
	bh1750_handle.mode = 2;
	bh1750_handle.sensitivity = 69;

	/* 数据初始化清零 */
	bh1750_handle.data.lux = 0;

	/* 发送上电指令 */
	BSP_BH1750_WriteCmd(BH1750_POWER_ON);

	/* 设置默认灵敏度 69 */
	BSP_BH1750_AdjustSensitivity(69);

	/* 设置为高分辨率模式2 (0.5 lux) */
	BSP_BH1750_ChageMode(2);

	return 0;
}

/**
 * @brief  BH1750 修改测量模式，决定测量分辨率
 * @param  mode: 测量模式 值域(1，2，3)
 * @retval None
 */
void BSP_BH1750_ChageMode(uint8_t mode)
{
	if (mode == 1) /* 连续高分测量模式1 */
	{
		BSP_BH1750_WriteCmd(BH1750_CON_H_RES);
		bh1750_handle.mode = 1; /* 测量模式1，分辨率 1 lux*/
	}
	else if (mode == 2) /* 连续高分测量模式2 */
	{
		BSP_BH1750_WriteCmd(BH1750_CON_H_RES2);
		bh1750_handle.mode = 2; /* 测量模式2, 分辨率 0.5 lux */
	}
	else if (mode == 3) /* 连续低分测量模式 */
	{
		BSP_BH1750_WriteCmd(BH1750_CON_L_RES);
		bh1750_handle.mode = 3; /* 测量模式3，低分辨率 4 lux*/
	}
}

/**
 * @brief  BH1750 调节测量灵敏度 (更改测量时间寄存器)
 * @param  sensitivity: 测量灵敏度
 * @retval None
 */
void BSP_BH1750_AdjustSensitivity(uint8_t sensitivity)
{
	if (sensitivity <= 31)
	{
		sensitivity = 31;
	}
	else if (sensitivity >= 254)
	{
		sensitivity = 254;
	}

	bh1750_handle.sensitivity = sensitivity;

	/* 按照芯片手册：高3位和低5位分两次发送 */
	BSP_BH1750_WriteCmd(0x40 | (bh1750_handle.sensitivity >> 5));
	BSP_BH1750_WriteCmd(0x60 | (bh1750_handle.sensitivity & 0x1F));

	/*　更改量程范围后，需要重新发送命令设置测量模式　*/
	BSP_BH1750_ChageMode(bh1750_handle.mode);
}

/**
 * @brief  更新 BH1750 的光照强度
 * @param  None
 * @retval 0-成功, 其他-失败
 */
uint8_t BSP_BH1750_UpdateData(void)
{
	uint8_t buffer[2] = {0};
	uint16_t raw_data = 0;
	float lux;

	if (bh1750_handle.interface.read(bh1750_handle.dev_address, buffer, 2, bh1750_handle.interface.intf_ptr) == 0)
	{
		raw_data = (uint16_t)((buffer[0] << 8) | buffer[1]);
	}
	else
	{
		return 1;
	}

	lux = (float)(raw_data * 5 * 69) / (6 * bh1750_handle.sensitivity);

	/* 模式2（H-Res Mode2），结果需要再除以 2 */
	if (bh1750_handle.mode == 2)
	{
		lux = lux / 2.0f;
	}

	bh1750_handle.data.lux = lux;

	return 0;
}

/**
 * @brief  获取 BH1750 的光照强度
 * @note   通过返回 const 指针，允许外部高效读取数据，同时防止外部直接修改私有结构体变量
 * @retval 指向 BH1750 数据结构体的 const 指针 (只读地址)
 */
const bh1750_data_t *BSP_BH1750_GetData(void)
{
    return &bh1750_handle.data;
}
