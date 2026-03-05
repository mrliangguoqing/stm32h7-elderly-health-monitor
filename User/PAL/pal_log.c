#include "pal_log.h"
#include "usart.h"

/**
 * @brief  重定向标准库函数 printf 到 USART1
 * @note   使用此功能，需要在 Keil 中勾选 Use MicroLIB
 * @param  ch: 要发送的字符
 * @param  f:  文件指针（在串口重定向中通常不使用，仅为匹配标准库原型）
 * @retval 返回发送成功的字符 ch
 */
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY); /* 发送一个字符 */
    return ch;                                                    /* 返回写入的字符 */
}