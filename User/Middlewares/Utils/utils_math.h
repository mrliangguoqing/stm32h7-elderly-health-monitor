/**
 * @file    utils_math.h
 * @brief   工具层 数学计算逻辑模块
 */

#ifndef __UTILS_MATH_H
#define __UTILS_MATH_H

#include <stdint.h>

void UTILS_Math_SplitFloat(float val, uint8_t decimal_places, int32_t *int_part, uint32_t *dec_part, int8_t *sign);

#endif
