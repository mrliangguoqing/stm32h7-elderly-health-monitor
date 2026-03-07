#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include <stdint.h>

#define RING_BUFFER_SIZE 512 /* 环形缓冲区大小 */

/**
 * @brief  环形缓冲区结构体
 */
typedef struct
{
    uint8_t buffer[RING_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} RingBuffer_t;

/* 通用缓冲区定义 */
extern RingBuffer_t g_uart_rx_fifo;
extern uint8_t g_rx_byte;

/* 函数声明 */
void RingBuffer_Write(RingBuffer_t *rb, uint8_t data);
int16_t RingBuffer_Read(RingBuffer_t *rb);
uint16_t RingBuffer_Available(RingBuffer_t *rb);
void RingBuffer_Clear(RingBuffer_t *rb);
uint8_t RingBuffer_ReadLine(RingBuffer_t *rb, char *pDest, uint16_t maxLen, uint16_t *pIndex);

#endif
