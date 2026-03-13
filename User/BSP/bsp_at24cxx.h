#ifndef __BSP_AT24CXX_H
#define __BSP_AT24CXX_H

#include <stdint.h>

#define AT24CXX_I2C_SLAVE_ADDRESS 0xA1 /* AT24Cxx I2C 从机地址 */

/* 函数声明 */
uint8_t AT24Cxx_WriteData(uint16_t mem_address, uint8_t *p_data,uint8_t size);
uint8_t AT24Cxx_ReadData(uint16_t mem_address, uint8_t *p_data, uint8_t size);

#endif

