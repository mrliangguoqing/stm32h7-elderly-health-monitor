/**
 * @file    bsp_mq5.c
 * @brief   MQ-5 可燃气体传感器驱动模块
 */

#include "bsp_mq5.h"
#include "bsp_dwt.h"

#include <string.h>

/* 内部静态实例 */
mq5_data_t mq5_dev;

/**
 * @brief  MQ-5 传感器模块初始化
 * @note   在系统启动时调用，用于清空结构体数据及缓冲区
 * @param  None
 * @retval None
 */
void BSP_MQ5_Init(void)
{
    memset(&mq5_dev, 0, sizeof(mq5_data_t));
}

/**
 * @brief  更新 MQ-5 原始采样值
 * @note   用于在 ADC 中断回调函数中调用
 * @param  raw: ADC 转换得到的 16 位原始数值 (0-65535)
 * @retval None
 */
void BSP_MQ5_UpdateRaw(uint16_t raw)
{
    mq5_dev.raw_data = raw;
}

/**
 * @brief  更新 MQ-5 的数据
 * @param  None
 * @retval 0-成功, 其他-失败
 */
uint8_t BSP_MQ5_UpdateData(void)
{
    /* 更新滤波缓冲区：将最新的原始值存入环形队列 */
    mq5_dev.filter_buf[mq5_dev.buf_index] = mq5_dev.raw_data;
    mq5_dev.buf_index = (mq5_dev.buf_index + 1) % MQ5_FILTER_SIZE;

    /* 计算滑动平均：先对原始值求和再平均，以减少单次计算中的浮点运算次数 */
    uint32_t sum = 0;
    for (int i = 0; i < MQ5_FILTER_SIZE; i++)
    {
        sum += mq5_dev.filter_buf[i];
    }
    float avg_raw = (float)sum / MQ5_FILTER_SIZE;

    /* 物理量转换：将 ADC 数值转换为实际电压值
       计算公式：(原始值 / 满量程) * 基准电压 * 硬件分压系数 */
    mq5_dev.voltage = (float)mq5_dev.raw_data * 3.3f / 65535.0f * MQ5_DIVIDER;
    mq5_dev.smooth_v = avg_raw * 3.3f / 65535.0f * MQ5_DIVIDER;

    /* 浓度计算预留
       此处可根据 MQ5 的阻值变化曲线（Rs/R0）计算具体的 PPM 浓度值
       log(ppm) = k * log(Rs/R0) + b
       mq5_dev.ppm = ... 
    */

    return 0;
}

/**
* @brief  获取 MQ-5 的数据
* @note   通过返回 const 指针，允许外部高效读取数据，同时防止外部直接修改私有结构体变量
* @retval 指向 MQ-5 数据结构体的 const 指针 (只读地址)
*/
const mq5_data_t *BSP_MQ5_GetData(void)
{
   return &mq5_dev;
}
