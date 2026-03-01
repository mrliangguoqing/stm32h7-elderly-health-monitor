/**
 * @file    bsp_aht30.c
 * @brief   aht30 温湿度驱动模块
 */

#include "bsp_aht30.h"

#include "i2c.h"

#define AHT30_TIMEOUT 100 /* I2C 发送或接受的超时时间 */

/* 内部静态实例 */
static aht30_handle_t aht30_handle;

/* 内部函数 */
static uint8_t AHT30_CRC8(uint8_t *message, uint8_t len);

/**
 * @brief  AHT30 I2C 底层写函数
 * @param  addr: 传感器 I2C 地址
 * @param  data: 待发送数据缓冲区首地址
 * @param  len:  待发送数据长度
 * @param  intf_ptr: I2C 硬件句柄指针 (I2C_HandleTypeDef *)
 * @retval 0-成功, 1-失败
 */
static uint8_t AHT30_I2C_Write(uint8_t addr, uint8_t *data, uint16_t len, void *intf_ptr)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)intf_ptr;
    return (HAL_I2C_Master_Transmit(hi2c, addr, data, len, AHT30_TIMEOUT) == HAL_OK) ? 0 : 1;
}

/**
 * @brief  AHT30 I2C 底层读函数
 * @param  addr: 传感器 I2C 地址
 * @param  data: 接收数据存储缓冲区首地址
 * @param  len:  待接收数据长度
 * @param  intf_ptr: I2C 硬件句柄指针 (I2C_HandleTypeDef *)
 * @retval 0-成功, 1-失败
 */
static uint8_t AHT30_I2C_Read(uint8_t addr, uint8_t *data, uint16_t len, void *intf_ptr)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)intf_ptr;
    return (HAL_I2C_Master_Receive(hi2c, addr, data, len, AHT30_TIMEOUT) == HAL_OK) ? 0 : 1;
}

/**
 * @brief  BSP 层 AHT30 传感器初始化
 * @note   完成硬件接口绑定、地址赋值并执行传感器上电初始化
 * @param  None
 * @retval 0-成功, 其他-失败
 */
uint8_t BSP_AHT30_Init(void)
{
    /* 绑定接口 */
    aht30_handle.interface.write = AHT30_I2C_Write;
    aht30_handle.interface.read = AHT30_I2C_Read;
    aht30_handle.interface.delay = BSP_DWT_DelayMs;
    aht30_handle.interface.intf_ptr = &hi2c1;

    /* 绑定从机地址参数 */
    aht30_handle.dev_addr = AHT30_I2C_ADDR;

    aht30_handle.interface.delay(5); /* 等待上电稳定 */

    return 0;
}

/**
 * @brief  更新 AHT30 的温湿度数据
 * @param  None
 * @retval 执行结果: 0-成功, 1-通讯错误, 2-CRC校验失败
 */
uint8_t BSP_AHT30_UpdateData(void)
{
    uint8_t start_cmd[] = {0xAC, 0x33, 0x00};
    uint8_t buffer[7] = {0};
    uint32_t srh_raw = 0, st_raw = 0;

    if (aht30_handle.interface.write(aht30_handle.dev_addr, start_cmd, 3, aht30_handle.interface.intf_ptr) != 0) /* 发送测量命令 */
    {
        return 1;
    }

    aht30_handle.interface.delay(85); /* 等待测量完成（手册要求 80ms） */

    if (aht30_handle.interface.read(aht30_handle.dev_addr, buffer, 7, aht30_handle.interface.intf_ptr) != 0) /* 读取 7 字节原始数据 */
    {
        return 1;
    }

    if (AHT30_CRC8(buffer, 6) != buffer[6]) /* CRC 校验 */
    {
        return 2;
    }

    aht30_handle.data.status = buffer[0];

    srh_raw = ((uint32_t)buffer[1] << 12) | ((uint32_t)buffer[2] << 4) | ((uint32_t)buffer[3] >> 4);
    st_raw = (((uint32_t)buffer[3] & 0x0F) << 16) | ((uint32_t)buffer[4] << 8) | (uint32_t)buffer[5];

    aht30_handle.data.humidity = (float)srh_raw * 100.0f / 1048576.0f;
    aht30_handle.data.temperature = (float)st_raw * 200.0f / 1048576.0f - 50.0f;

    return 0;
}

/**
 * @brief  获取 AHT30 的温湿度数据
 * @note   通过返回 const 指针，允许外部高效读取数据，同时防止外部直接修改私有结构体变量
 * @retval 指向 AHT30 数据结构体的 const 指针 (只读地址)
 */
const aht30_data_t *BSP_AHT30_GetData(void)
{
    return &aht30_handle.data;
}

/**
 * @brief  CRC8 校验计算 (多项式: X8+X5+X4+1 -> 0x31, 初始值: 0xFF)
 * @param  message: 指向包含传感器返回数据的数组首地址
 * @param  len: 需要参与校验的数据字节数
 * @retval 计算出来的 8 位 CRC 校验码
 */
static uint8_t AHT30_CRC8(uint8_t *message, uint8_t len)
{
    uint8_t crc = 0xFF;

    for (uint8_t j = 0; j < len; j++)
    {
        crc ^= message[j];

        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = (crc << 1);
        }
    }

    return crc;
}
