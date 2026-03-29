/**
 * @file    bsp_a7670c.h
 * @brief   A7670C 4G 模块驱动模块头文件
 */

#ifndef __BSP_A7670C_H
#define __BSP_A7670C_H

#include <stdint.h>

#define A7670C_UART huart3 /* A7670C 连接的串口 */

/* A7670C 短信相关 AT 指令宏定义 */
#define A7670C_AT_CMGF_1 "AT+CMGF=1\r\n"             /* 设置短信格式为文本模式 (TEXT Mode) */
#define A7670C_AT_CSMP_BASE "AT+CSMP=49,167,0,8\r\n" /* 设置短信文本模式参数 (支持 Unicode/UCS2) */
#define A7670C_AT_CMGS "AT+CMGS=\"%s\"\r\n"          /* 发送短信指令格式 (需填入手机号) */
#define A7670C_CTRL_Z "\x1A"                         /* 短信结束符: Ctrl+Z (十六进制 0x1A) */

uint8_t BSP_A7670C_SendEmergencySMS(void);

#endif
