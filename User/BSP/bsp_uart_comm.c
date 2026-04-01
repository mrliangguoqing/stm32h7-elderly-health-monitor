/**
 * @file    bsp_uart_comm.c
 * @brief   串口接收不定长数据模块驱动模块
 */

#include "bsp_uart_comm.h"
#include "usart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "app_voice_service.h"

#define VOICE_UART_HANDLE (&huart4)
#define VOICE_UART_INSTANCE USART4

/* 实例化结构体 */
uart_control_t voice_conn = {VOICE_UART_HANDLE, {0}, 0, 0};
// uart_control_t radar_conn = {&huart1, {0}, 0, 0};

/**
 * @brief  初始化串口中断配置 (针对 H7/通用系列)
 * @note   手动开启串口的接收非空中断 (RXNE) 和总线空闲中断 (IDLE)
 * @retval 无
 */
void BSP_UART_Init(void)
{
    /* 开启接收中断 (RXNE) 和 空闲中断 (IDLE) */
    __HAL_UART_ENABLE_IT(voice_conn.huart, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(voice_conn.huart, UART_IT_IDLE);

    // __HAL_UART_ENABLE_IT(radar_conn.huart, UART_IT_RXNE);
    // __HAL_UART_ENABLE_IT(radar_conn.huart, UART_IT_IDLE);
}

/**
 * @brief  通用的 STM32H7 串口中断解析函数
 * @param  conn: 指向串口控制结构体 (uart_control_t) 的指针
 * @note   1. 兼容 H7 系列的 ISR/RDR/ICR 寄存器操作
 *         2. 集成 IDLE (空闲中断) 实现不定长数据接收
 *         3. 接收完成通过 FreeRTOS 二值信号量 (Binary Semaphore) 唤醒处理任务
 */
void BSP_UART_Process_IRQHandler(uart_control_t *conn)
{
    uint32_t isr_val = conn->huart->Instance->ISR; // H7 使用 ISR 寄存器

    /* 处理接收中断 (RXNE) */
    if ((isr_val & UART_FLAG_RXNE) != RESET)
    {
        /* 读取 RDR 自动清除 RXNE 位 */
        uint8_t res = (uint8_t)(conn->huart->Instance->RDR & 0xFF);

        if (conn->rx_cnt < UART_RX_MAX_LEN)
        {
            conn->rx_buf[conn->rx_cnt++] = res;
        }
    }

    /* 处理空闲中断 (IDLE) */
    if ((isr_val & UART_FLAG_IDLE) != RESET)
    {
        /* 向 ICR 寄存器写相应位，清除 IDLE 位 */
        __HAL_UART_CLEAR_IT(conn->huart, UART_CLEAR_IDLEF);

        if (conn->rx_cnt > 0)
        {
            conn->rx_complete = 1; /* 标记收到完整一帧 */

            if ((conn->huart == VOICE_UART_HANDLE) && (xVoiceDataReadySem != NULL))
            {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                /* 发送信号量唤醒 Task */
                xSemaphoreGiveFromISR(xVoiceDataReadySem, &xHigherPriorityTaskWoken);
                /* 强制上下文切换 */
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }

    /* 错误处理 */
    if ((isr_val & (UART_FLAG_ORE | UART_FLAG_NE | UART_FLAG_FE)) != RESET)
    {
        __HAL_UART_CLEAR_IT(conn->huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF);
    }
}

/**
 * @brief  重置串口接收缓冲区状态
 * @param  conn: 指向串口控制结构体 (uart_control_t) 的指针
 * @note   清零接收计数器并重置接收完成标志位，为下一帧数据接收做准备
 */
void BSP_UART_ClearBuffer(uart_control_t *conn)
{
    conn->rx_cnt = 0;
    conn->rx_complete = 0;
}
