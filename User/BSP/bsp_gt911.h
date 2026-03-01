#ifndef __BSP_GT911_H
#define __BSP_GT911_H

#include <stdint.h>
#include <stdbool.h>

#define GT911_TOUCH_NUMBER 5 /* GT911 输出触点个数上限：1~5 */

/* GT911 寄存器地址定义 */
#define GT911_IIC_WRITE_ADDR 0xBA          // 写地址
#define GT911_IIC_READ_ADDR 0xBB           // 读地址
#define GT911_PRODUCT_ID_REG_ADDR 0x8140   // ID寄存器
#define GT911_TOUCH_STATUS_REG_ADDR 0x814E // 坐标状态寄存器

/**
 * @brief  GT911 触摸屏单个触摸点数据结构体
 */
typedef struct
{
    uint16_t x;    /* 触摸点 X 坐标 */
    uint16_t y;    /* 触摸点 Y 坐标 */
    uint16_t size; /* 触摸点大小 */
} gt911_touch_point_t;

/**
 * @brief  GT911 触摸屏的状态与数据结构体
 */
typedef struct
{
    uint16_t max_x; /* 触摸屏 X 轴最大量程 */
    uint16_t max_y; /* 触摸屏 Y 轴最大量程 */

    /* 直接读取 0x814E 的原始字节
       bit0~bit3->number of touch points
       bit4~bit5->Reserved
       bit6->large detect
       bit7->buffer status
    */
    uint8_t raw_status;

    uint8_t touch_number; /* 软件记录：当前有效的触摸点数量 (0-5) */
    bool is_pressed;      /* 软件判定：当前是否有手指按下 (逻辑开关) */

    gt911_touch_point_t points[GT911_TOUCH_NUMBER]; /* 触摸点数据 */
} gt911_handle_t;

/* 函数声明 */
uint8_t BSP_GT911_Init(void);
uint8_t BSP_GT911_ReadId(void);
uint8_t BSP_GT911_BindLCD(void);
uint8_t BSP_GT911_Scan(void);
void BSP_GT911_Test(void);

bool BSP_GT911_IsPressed(void);
const gt911_touch_point_t *BSP_GT911_GetTouchPoint(void);

#endif
