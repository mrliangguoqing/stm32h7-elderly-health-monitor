#include "bsp_aht30.h"

#define AHT30_TIMEOUT 100   /* I2C 发送或接受的超时时间 */

/* 内部静态实例 */
static AHT30_t h_aht30;

/**
 * @brief  AHT30 I2C 底层写函数适配 (基于 STM32 HAL 库)
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
 * @brief  AHT30 I2C 底层读函数适配 (基于 STM32 HAL 库)
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
    // 绑定接口
    h_aht30.interface.write = AHT30_I2C_Write;
    h_aht30.interface.read = AHT30_I2C_Read;
    h_aht30.interface.delay = BSP_DWT_DelayMs;
    h_aht30.interface.intf_ptr = &hi2c1;

    // 绑定参数
    h_aht30.dev_addr = AHT30_I2C_ADDR;

    return AHT30_Init(&h_aht30); // 调用 aht30.c 中的逻辑
}

/**
 * @brief  BSP 层 AHT30 数据更新触发
 * @note   通过 I2C 触发测量并读取最新温湿度数据至内部结构体
 * @param  None
 * @retval 0-成功, 1-通讯错误, 2-CRC校验失败
 */
uint8_t BSP_AHT30_Update(void)
{
    return AHT30_Get_Temp_Hum(&h_aht30);
}

/**
 * @brief  获取 AHT30 缓存的温湿度数据
 * @param  None
 * @retval 包含温度、湿度及状态字的 AHT30_Data_t 结构体
 */
AHT30_Data_t BSP_AHT30_GetData(void)
{
    return h_aht30.data;
}
