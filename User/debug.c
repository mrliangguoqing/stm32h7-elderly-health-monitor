#include "debug.h"

/*
* 重定向 fputc 到 UART（以 USART1 为例），可以直接使用 printf() 打印到串口
* 若使用此功能，需要将 Use MicroLIB 打开，打开方式如下
* Options for Target -> Target -> Code Generatoion -> Use MicroLIB
*/
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY); // 发送一个字符
    return ch;                                                    // 返回写入的字符（符合标准）
}