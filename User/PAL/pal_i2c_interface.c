#include "pal_i2c_interface.h"
#include "i2c.h"

/**
 * @brief  I2C 写函数
 * @param  dev_address: 传感器 I2C 地址
 * @param  p_data: 待发送数据缓冲区首地址
 * @param  size:  待发送的数据量
 * @param  intf_ptr: I2C 硬件句柄指针 (I2C_HandleTypeDef *)
 * @retval 0-成功, 1-失败
 */
uint8_t PAL_STM32_I2C_Write(uint8_t dev_address, uint8_t *p_data, uint16_t size, void *intf_ptr)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)intf_ptr;
    return (HAL_I2C_Master_Transmit(hi2c, dev_address, p_data, size, 100) == HAL_OK) ? 0 : 1;
}

/**
 * @brief  I2C 写函数
 * @param  dev_address: 传感器 I2C 地址
 * @param  p_data: 待发送数据缓冲区首地址
 * @param  size:  待发送的数据量
 * @param  intf_ptr: I2C 硬件句柄指针 (I2C_HandleTypeDef *)
 * @retval 0-成功, 1-失败
 */
uint8_t PAL_STM32_I2C_Read(uint8_t dev_address, uint8_t *p_data, uint16_t size, void *intf_ptr)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)intf_ptr;
    return (HAL_I2C_Master_Receive(hi2c, dev_address, p_data, size, 100) == HAL_OK) ? 0 : 1;
}
