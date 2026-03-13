#include "bsp_at24cxx.h"
#include "i2c.h"

/**
 * @brief  AT24Cxx I2C 写寄存器
 * @param  mem_address 写寄存器的地址
 * @param  p_data 发送数据缓冲区首地址
 * @param  size  要发送的数据量
 * @retval 0-成功, 1-失败
 */
uint8_t AT24Cxx_WriteData(uint16_t mem_address, uint8_t *p_data,uint8_t size)
{
    return (HAL_I2C_Mem_Write(&hi2c4, AT24CXX_I2C_SLAVE_ADDRESS, mem_address, I2C_MEMADD_SIZE_16BIT, p_data, size, 100) == HAL_OK) ? 0 : 1;
}

/**
 * @brief  AT24Cxx I2C 读寄存器
 * @param  mem_address 读寄存器的地址
 * @param  p_data 接收数据存储缓冲区首地址
 * @param  size  要接收的数据量
 * @retval 0-成功, 1-失败
 */
uint8_t AT24Cxx_ReadData(uint16_t mem_address, uint8_t *p_data, uint8_t size)
{
    return (HAL_I2C_Mem_Read(&hi2c4, AT24CXX_I2C_SLAVE_ADDRESS, mem_address, I2C_MEMADD_SIZE_16BIT, p_data, size, 100) == HAL_OK) ? 0 : 1;
}
