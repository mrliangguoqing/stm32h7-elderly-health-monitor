/**
 * @file    bsp_max30102.c
 * @brief   Max30102 心率血氧模块
 */

#include "bsp_max30102.h"
#include "bsp_dwt.h"
#include "i2c.h"

/* 内部静态实例 */
max30102_handle_t max30102_handle;

/**
 * @brief  MAX30102 I2C 写寄存器
 * @param  mem_address 写寄存器的地址
 * @param  data 待发送的数据
 * @retval 0-成功, 1-失败
 */
static uint8_t MAX30102_WriteData(uint16_t mem_address, uint8_t data)
{
    return (HAL_I2C_Mem_Write(&hi2c3, MAX30102_I2C_SLAVE_ADDRESS, mem_address, I2C_MEMADD_SIZE_8BIT, &data, 1, 100) == HAL_OK) ? 0 : 1;
}

/**
 * @brief  MAX30102 I2C 读寄存器
 * @param  mem_address 读寄存器的地址
 * @param  data 接收数据存储缓冲区首地址
 * @param  size  要接收的数据量
 * @retval 0-成功, 1-失败
 */
static uint8_t MAX30102_ReadData(uint16_t mem_address, uint8_t *pData, uint8_t size)
{
    return (HAL_I2C_Mem_Read(&hi2c3, MAX30102_I2C_SLAVE_ADDRESS, mem_address, I2C_MEMADD_SIZE_8BIT, pData, size, 100) == HAL_OK) ? 0 : 1;
}

/**
 * @brief  MAX30102 复位
 * @param  None
 * @retval None
 */
void BSP_MAX30102_Reset(void)
{
    MAX30102_WriteData(MAX30102_MODE_CONFIG_REG_ADDR, 0x40);
    BSP_DWT_DelayMs(50); /* 给足复位等待时间 */
}

/**
 * @brief  MAX30102 传感器初始化
 * @param  None
 * @retval 0-成功, 其他-失败
 */
uint8_t BSP_MAX30102_Init(void)
{
    BSP_MAX30102_Reset();

    /* 配置中断使能寄存器 */
    MAX30102_WriteData(MAX30102_INTR_ENABLE_1_REG_ADDR, 0xc0);
    MAX30102_WriteData(MAX30102_INTR_ENABLE_2_REG_ADDR, 0x00);

    /* 重置 FIFO 相关指针 */
    MAX30102_WriteData(MAX30102_OVF_COUNTER_REG_ADDR, 0x00);
    MAX30102_WriteData(MAX30102_FIFO_RD_PTR_REG_ADDR, 0x00);
    MAX30102_WriteData(MAX30102_FIFO_WR_PTR_REG_ADDR, 0x00);

    /* 配置 FIFO (采样平均、溢出滚动等) */
    MAX30102_WriteData(MAX30102_FIFO_CONFIG_REG_ADDR, 0x0F);

    /* 配置工作模式 (0x03 为 SpO2 模式: 红光+红外光) */
    MAX30102_WriteData(MAX30102_MODE_CONFIG_REG_ADDR, 0x03);

    /* 配置 SpO2 细节 (ADC 范围、采样率、脉宽) */
    MAX30102_WriteData(MAX30102_SPO2_CONFIG_REG_ADDR, 0x27);

    /* 配置 LED 电流/亮度 (控制穿透深度) */
    MAX30102_WriteData(MAX30102_LED1_PA_REG_ADDR, 0x24);  /* RED LED */
    MAX30102_WriteData(MAX30102_LED2_PA_REG_ADDR, 0x24);  /* IR LED */
    MAX30102_WriteData(MAX30102_PILOT_PA_REG_ADDR, 0x7F); /* 邻近检测电流 */

    return 0;
}

/**
 * @brief  读取 MAX30102 FIFO 中的单组采样数据
 * @param  red_led 指向 uint32_t 的指针，用于存储解析后的红光通道原始数值
 * @param  ir_led  指向 uint32_t 的指针，用于存储解析后的红外光通道原始数值
 * @note   1. 必须在收到 GPIO 中断信号后调用此函数，否则可能读到重复或无效数据。
 *         2. MAX30102 的采样精度最高为 18-bit，多余的高位会被掩码 0x03FFFF 滤除。
 */
void BSP_MAX30102_ReadFifo(uint32_t *red_led, uint32_t *ir_led)
{
    uint8_t data[6];
    uint8_t temp;

    /* 读取并清除中断状态 */
    MAX30102_ReadData(MAX30102_INTR_STATUS_1_REG_ADDR, &temp, 1);
    MAX30102_ReadData(MAX30102_INTR_STATUS_2_REG_ADDR, &temp, 1);

    /* 连续读取 6 个字节 */
    if (MAX30102_ReadData(MAX30102_FIFO_DATA_REG_ADDR, data, 6) == 0)
    {
        /* 解析 RED LED (18位数据) */
        *red_led = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | (uint32_t)data[2];
        *red_led &= 0x03FFFF;

        /* 解析 IR LED (18位数据) */
        *ir_led = ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | (uint32_t)data[5];
        *ir_led &= 0x03FFFF;
    }
}
