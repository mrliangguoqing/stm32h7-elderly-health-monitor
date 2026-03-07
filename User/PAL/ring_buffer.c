#include "ring_buffer.h"
#include "pal_log.h"
#include "usart.h"

/* 变量定义 */
RingBuffer_t g_uart_rx_fifo = {0}; /* 串口接收环形缓冲区结构体实例 */
uint8_t g_rx_byte = 0;             /* 中断接收到的单字节缓存 */

/**
 * @brief  向环形缓冲区写入一个字节数据
 * @param  rb: 指向环形缓冲区结构体的指针
 * @param  data: 准备写入的 8 位数据字节
 * @retval None
 */
void RingBuffer_Write(RingBuffer_t *rb, uint8_t data)
{
    /* 计算下一个写入位置，实现循环覆盖逻辑 */
    uint16_t next = (rb->head + 1) % RING_BUFFER_SIZE;

    /* 检查缓冲区是否未满 */
    if (next != rb->tail)
    {
        /* 将数据存入当前头指针位置 */
        rb->buffer[rb->head] = data;
        
        /* 更新头指针到下一个位置 */
        rb->head = next;
    }
    /* 若 (next == rb->tail)，此处隐含了溢出处理，当前选择直接丢弃该字节 */
}

/**
 * @brief  从环形缓冲区读取一个字节数据
 * @param  rb: 指向环形缓冲区结构体的指针
 * @retval 读取到的数据(0-255)，若缓冲区为空则返回 -1
 */
int16_t RingBuffer_Read(RingBuffer_t *rb)
{
    /* 检查缓冲区是否为空 */
    if (rb->head == rb->tail)
    {
        return -1;
    }

    /* 取出尾指针指向的数据 */
    uint8_t data = rb->buffer[rb->tail];

    /* 更新尾指针位置，实现回环 */
    rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;

    return data;
}

/**
 * @brief  获取环形缓冲区中当前可读的数据长度
 * @param  rb: 指向环形缓冲区结构体的指针
 * @retval 缓冲区中有效数据的字节数
 */
uint16_t RingBuffer_Available(RingBuffer_t *rb)
{
    /* 利用取模运算计算头尾指针之间的间距，得出有效数据量 */
    return (RING_BUFFER_SIZE + rb->head - rb->tail) % RING_BUFFER_SIZE;
}

/**
 * @brief  清空环形缓冲区
 * @param  rb: 指向环形缓冲区结构体的指针
 * @retval None
 */
void RingBuffer_Clear(RingBuffer_t *rb)
{
    /* 进入临界区：关闭全局中断，防止清除过程中发生数据压入 */
    __disable_irq();
    
    rb->head = 0;
    rb->tail = 0;
    
    /* 退出临界区：恢复全局中断 */
    __enable_irq();
}


/**
 * @brief  从环形缓冲区中读取一行数据（以换行符 \n 结束）
 * @param  rb: 指向环形缓冲区结构体的指针
 * @param  pDest: 目标存储缓冲区的地址
 * @param  maxLen: 目标缓冲区的最大长度（防止溢出）
 * @param  pIndex: 指向当前行读取进度的索引指针（必须是外部静态变量，用于记录多次调用间的状态）
 * @retval 0: 成功读取到完整的一行; 1: 数据尚未收全或缓冲区已空
 */
uint8_t RingBuffer_ReadLine(RingBuffer_t *rb, char *pDest, uint16_t maxLen, uint16_t *pIndex)
{
    /* 只要环形缓冲区内存在未处理的数据，则持续进行读取操作 */
    while (RingBuffer_Available(rb))
    {
        /* 从缓冲区取出一个字节数据 */
        int16_t data = RingBuffer_Read(rb);

        /* 检查读取结果：若返回 -1 则说明缓冲区为空，终止本次处理 */
        if (data == -1)
        {
            break;
        }

        char c = (char)data;

        /* 忽略回车符 '\r'，用于兼容不同的换行标准 */
        if (c == '\r')
        {
            continue;
        }

        /* 检测到换行符 '\n'，表示一帧数据结束 */
        if (c == '\n')
        {
            /* 若当前索引为 0，说明收到的是空行或连续换行，不做处理继续解析 */
            if (*pIndex == 0)
            {
                continue;
            }
            
            /* 收到换行且有内容，在字符串末尾添加结束符 \0 */
            pDest[*pIndex] = '\0';
            
            /* 重置进度索引，为解析下一行数据做准备 */
            *pIndex = 0;

            return 0; /* 返回成功标志 */
        }

        /* 将接收到的有效字符存入目标缓冲区，前提是不超过缓冲区最大限制 */
        if (*pIndex < maxLen - 1)
        {
            pDest[(*pIndex)++] = c;
        }
        else
        {
            /* 发生缓冲区溢出：强制清空当前索引，防止越界并丢弃该行错误数据 */
            *pIndex = 0;
        }
    }

    /* 缓冲区数据已处理完，但仍未匹配到换行符，返回 1 待下次继续处理 */
    return 1;
}
