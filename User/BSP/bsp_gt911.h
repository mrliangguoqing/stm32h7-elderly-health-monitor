#ifndef __BSP_GT911_H
#define __BSP_GT911_H

#include <stdint.h>

#define GT911_TOUCH_NUMBER 5 /* GT911 输出触点个数上限：1~5 */

/* GT911 寄存器地址定义 */
#define GT911_IIC_WRITE_ADDR 0xBA          // 写地址
#define GT911_IIC_READ_ADDR 0xBB           // 读地址
#define GT911_PRODUCT_ID_REG_ADDR 0x8140   // ID寄存器
#define GT911_TOUCH_STATUS_REG_ADDR 0x814E // 坐标状态寄存器

/**
 * @brief  GT911 触摸数据结构体
 */
typedef struct
{
    uint16_t x[GT911_TOUCH_NUMBER]; /* 触摸点 X 坐标 */
    uint16_t y[GT911_TOUCH_NUMBER]; /* 触摸点 Y 坐标 */
    uint8_t sta;                    /* 触摸状态、触摸点的数量 */
} _gt911_dev;

extern _gt911_dev gt911dev;

/* 函数声明 */
uint8_t BSP_GT911_Init(void);
uint8_t BSP_GT911_ReadId(void);
uint8_t BSP_GT911_Scan(void);
void BSP_GT911_Test(void);

#endif
