#ifndef __PAL_I2C_INTERFACE_H
#define __PAL_I2C_INTERFACE_H

#include <stdint.h>

/* 定义函数指针类型 */
/**
 * @brief  I2C 写函数指针类型定义
 * @param  dev_address: 传感器 I2C 地址
 * @param  p_data: 待发送数据缓冲区首地址
 * @param  size:  待发送的数据量
 * @param  intf_ptr: I2C 硬件句柄指针 (I2C_HandleTypeDef *)
 * @retval 0-成功, 1-失败
 */
typedef uint8_t (*i2c_write_fptr_t)(uint8_t dev_address, uint8_t *p_data, uint16_t size, void *intf_ptr);

/**
 * @brief  I2C 读函数指针类型定义
 * @param  dev_address: 传感器 I2C 地址
 * @param  p_data: 待发送数据缓冲区首地址
 * @param  size:  待发送的数据量
 * @param  intf_ptr: I2C 硬件句柄指针 (I2C_HandleTypeDef *)
 * @retval 0-成功, 1-失败
 */
typedef uint8_t (*i2c_read_fptr_t)(uint8_t dev_address, uint8_t *p_data, uint16_t size, void *intf_ptr);

/**
 * @brief  I2C 延时函数指针类型定义
 * @param  ms: 需要延时的毫秒数
 */
typedef void (*delay_ms_fptr_t)(uint32_t ms);

/**
 * @brief  I2C 硬件接口适配结构体
 */
typedef struct
{
    i2c_write_fptr_t write; /* 平台相关的 I2C 写函数 */
    i2c_read_fptr_t read;   /* 平台相关的 I2C 读函数 */
    delay_ms_fptr_t delay;  /* 平台相关的毫秒级延时函数 */
    void *intf_ptr;         /* 存放如 I2C_HandleTypeDef* 的句柄 */
} pal_i2c_interface_t;

/* 函数声明 */
uint8_t PAL_I2C_Write(uint8_t dev_address, uint8_t *p_data, uint16_t size, void *intf_ptr);
uint8_t PAL_I2C_Read(uint8_t dev_address, uint8_t *p_data, uint16_t size, void *intf_ptr);

#endif
