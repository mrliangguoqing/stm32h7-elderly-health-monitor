#ifndef __AHT30_H
#define __AHT30_H

#include <stdint.h>

#define AHT30_I2C_ADDR 0x70 /* AHT30 I2C 从机地址 */

/* 定义函数指针类型 */
/**
 * @brief  AHT30 I2C 读写函数指针类型定义
 * @param  addr: 7 位设备 I2C 地址
 * @param  data: 待发送或接收的数据缓冲区指针
 * @param  len:  数据字节长度
 * @param  intf_ptr: 接口私有指针，用于传递 I2C 硬件句柄 I2C_HandleTypeDef
 * @retval 0-成功, 其他-失败
 */
typedef uint8_t (*AHT30_I2C_Ptr)(uint8_t addr, uint8_t *data, uint16_t len, void *intf_ptr);

/**
 * @brief  AHT30 延时函数指针类型定义
 * @param  ms: 需要延时的毫秒数
 */
typedef void (*AHT30_Delay_Ptr)(uint32_t ms);

/**
 * @brief  AHT30 硬件接口配置结构体
 */
typedef struct
{
    AHT30_I2C_Ptr write;   /* 平台相关的 I2C 写函数 */
    AHT30_I2C_Ptr read;    /* 平台相关的 I2C 读函数 */
    AHT30_Delay_Ptr delay; /* 平台相关的毫秒级延时函数 */
    void *intf_ptr;        /* 指向 I2C 硬件句柄的指针 */
} AHT30_Interface_t;

/**
 * @brief  AHT30 传感器测量数据结构体
 */
typedef struct
{
    float temperature; /* 转换后的温度值 (单位: ℃) */
    float humidity;    /* 转换后的相对湿度值 (单位: %RH) */
    uint8_t status;    /* 传感器返回的状态字 */
} AHT30_Data_t;

/**
 * @brief  AHT30 设备句柄结构体
 */
typedef struct
{
    AHT30_Interface_t interface; /* 硬件接口适配层 */
    AHT30_Data_t data;           /* 存储最新的测量结果 */
    uint8_t dev_addr;            /* 设备的 I2C 写入地址 */
} AHT30_t;

/* 函数声明 */
uint8_t AHT30_Init(AHT30_t *dev);
uint8_t AHT30_Get_Temp_Hum(AHT30_t *dev);

#endif
