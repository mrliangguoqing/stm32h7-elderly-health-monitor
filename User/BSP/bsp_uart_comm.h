/**
 * @file    bsp_uart_comm.h
 * @brief   串口接收不定长数据模块驱动模块头文件
 */

#ifndef __BSP_UART_COMM_H
#define __BSP_UART_COMM_H

#include "main.h"

/* 缓冲区大小 */
#define UART_RX_MAX_LEN 256

typedef struct
{
    UART_HandleTypeDef *huart;       /* 指向硬件句柄 (如 &huart1) */
    uint8_t rx_buf[UART_RX_MAX_LEN]; /* 独立接收缓冲区 */
    uint16_t rx_cnt;                 /* 当前接收计数值 */
    volatile uint8_t rx_complete;    /* 帧接收完成标志 */
} uart_control_t;

/* 函数声明 */
void BSP_UART_Init(void);
void BSP_UART_Process_IRQHandler(uart_control_t *conn);
void BSP_UART_ClearBuffer(uart_control_t *conn);

extern uart_control_t voice_conn;
extern uart_control_t radar_conn;

#endif
