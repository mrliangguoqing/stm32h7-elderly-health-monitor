/**
 * @file    utils_math.c
 * @brief   工具层 数学计算逻辑模块
 */

#include "utils_math.h"

/**
 * @brief  浮点数高效拆分函数 (优化版)
 * @param  val: 待处理浮点数
 * @param  decimal_places: 保留位数 (0-9)
 * @param  int_part: 返回整数部分 (带符号，防止后续运算溢出)
 * @param  dec_part: 返回小数部分 (无符号，语义更精准)
 * @param  sign: 返回符号 (1为正，-1为负)
 */
void UTILS_Math_SplitFloat(float val, uint8_t decimal_places, int32_t *int_part, uint32_t *dec_part, int8_t *sign)
{
    /* 静态查找表 (最高支持 9 位，涵盖 float 精度上限) */
    static const uint32_t pow10_table[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

    if (decimal_places > 9) /* 限制范围防止数组越界 */
    {
        decimal_places = 9;
    }

    uint32_t multiplier = pow10_table[decimal_places]; /* 获取倍数 */

    /* 符号处理 */
    if (val < 0.0f)
    {
        *sign = -1;
        val = -val;
    }
    else
    {
        *sign = 1;
    }

    *int_part = (int32_t)val; /* 提取整数部分 */

    /* 提取小数部分并四舍五入 */
    float f_dec = (val - (float)(*int_part)) * (float)multiplier;
    *dec_part = (uint32_t)(f_dec + 0.5f);

    if (*dec_part >= multiplier) /* 进位补偿逻辑 */
    {
        *dec_part = 0;
        (*int_part)++;
    }
}